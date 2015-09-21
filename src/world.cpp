#include "world.hpp"
#include "player.hpp"
#include "math_lib/m_math.h"
#include "renderer/i_world_renderer.hpp"
#include "settings.hpp"
#include "settings_keys.hpp"

h_World::h_World(
	const h_SettingsPtr& settings )
	: settings_( settings )
	, chunk_loader_( "world" )
	, phys_tick_count_(0)
	, terminated_(false)
{
	InitNormalBlocks();

	chunk_number_x_= std::max( std::min( settings_->GetInt( h_SettingsKeys::chunk_number_x, 14 ), H_MAX_CHUNKS ), H_MIN_CHUNKS );
	chunk_number_y_= std::max( std::min( settings_->GetInt( h_SettingsKeys::chunk_number_y, 12 ), H_MAX_CHUNKS ), H_MIN_CHUNKS );
	longitude_= -(chunk_number_x_/2);
	latitude_= -(chunk_number_y_/2);
	//chunk_matrix_size_x_log2= m_Math::NearestPOTLog2( chunk_number_x );
	//chunk_matrix_size_x= 1 << chunk_matrix_size_x_log2;

	// chunks= new h_Chunk*[ chunk_matrix_size_x * chunk_number_y ];
	// for( int i=0; i< chunk_number_x * chunk_number_y; i++ )
	//    chunks[i]= new h_Chunk( this, (i%chunk_number_x) + longitude, (i/chunk_number_x) + latitude );
	for( unsigned int i= 0; i< chunk_number_x_; i++ )
		for( unsigned int j= 0; j< chunk_number_y_; j++ )
		{
			chunks_[ i + j * H_MAX_CHUNKS ]= LoadChunk( i+longitude_, j+latitude_);
			//new h_Chunk( this, i + longitude, j + latitude );
		}

	LightWorld();

	//phys_thread_.setStackSize( 16 * 1024 * 1024 );//inrease stack for recursive methods( lighting, blasts, etc )
}

h_World::~h_World()
{
	if( phys_thread_ )
	{
		terminated_.store(true);
		phys_thread_->join();
		phys_thread_.reset();
	}

	for( unsigned int x= 0; x< chunk_number_x_; x++ )
		for( unsigned int y= 0; y< chunk_number_y_; y++ )
		{
			SaveChunk( GetChunk(x,y) );
			delete GetChunk(x,y);
		}
}

void h_World::AddBuildEvent( short x, short y, short z, h_BlockType block_type )
{
	action_queue_mutex_.lock();

	h_WorldAction act;
	act.type= ACTION_BUILD;
	act.block_type= block_type;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	action_queue_[0].push( act );

	action_queue_mutex_.unlock();
}

void h_World::AddDestroyEvent( short x, short y, short z )
{
	action_queue_mutex_.lock();

	h_WorldAction act;
	act.type= ACTION_DESTROY;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	action_queue_[0].push( act );

	action_queue_mutex_.unlock();
}

void h_World::Blast( short x, short y, short z, short radius )
{
	if( !InBorders( x, y, z ) )
		return;

	for( short k= z, r=radius; k< z+radius; k++, r-- )
		BlastBlock_r( x, y, k, r );
	for( short k= z-1, r=radius-1; k> z-radius; k--, r-- )
		BlastBlock_r( x, y, k, r );

	for( short i= x - radius; i< x+radius; i++ )
		for( short j= y - radius; j< y+radius; j++ )
			for( short k= z - radius; k< z+radius; k++ )
				RelightBlockRemove( i, j, k );

	UpdateInRadius( x, y, radius );
}

void h_World::StartUpdates()
{;
	phys_thread_.reset( new std::thread( &h_World::PhysTick, this ) );
}

void h_World::Save()
{
	for( unsigned int x= 0; x< chunk_number_x_; x++ )
		for( unsigned int y= 0; y< chunk_number_y_; y++ )
			SaveChunk( GetChunk(x,y) );
	chunk_loader_.ForceSaveAllChunks();
}

void h_World::Build( short x, short y, short z, h_BlockType block_type )
{
	if( !InBorders( x, y, z ) )
		return;
	if( !CanBuild( x, y, z ) )
		return;

	short X= x>> H_CHUNK_WIDTH_LOG2, Y = y>> H_CHUNK_WIDTH_LOG2;

	x&= H_CHUNK_WIDTH - 1;
	y&= H_CHUNK_WIDTH - 1;
	if( block_type == WATER )
	{
		h_LiquidBlock* b;
		h_Chunk* ch=GetChunk( X, Y );
		ch-> SetBlockAndTransparency( x, y, z, b= ch->NewWaterBlock(),
									  TRANSPARENCY_LIQUID );

		b->x_= x;
		b->y_= y;
		b->z_= z;
	}
	else if( block_type == FIRE_STONE )
	{
		h_Chunk* ch=GetChunk( X, Y );
		h_LightSource* s= ch->NewLightSource( x, y, z, FIRE_STONE );
		s->SetLightLevel( H_MAX_FIRE_LIGHT );
		ch->SetBlockAndTransparency( x, y, z, s, TRANSPARENCY_SOLID );
		AddFireLight_r( x + X* H_CHUNK_WIDTH, y + Y* H_CHUNK_WIDTH, z, H_MAX_FIRE_LIGHT );
	}
	else
		GetChunk( X, Y )->
		SetBlockAndTransparency( x, y, z, NormalBlock( block_type ),
								 NormalBlock( block_type )->Transparency() );

	short r= 1;
	if( block_type == WATER )
	{
	}
	else
		r= RelightBlockAdd( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, z ) + 1;

	UpdateInRadius( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, r );
	UpdateWaterInRadius( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, r );
}

void h_World::Destroy( short x, short y, short z )
{
	if( !InBorders( x, y, z ) )
		return;

	short X= x>> H_CHUNK_WIDTH_LOG2, Y = y>> H_CHUNK_WIDTH_LOG2;
	x&= H_CHUNK_WIDTH - 1;
	y&= H_CHUNK_WIDTH - 1;

	h_Chunk* ch=GetChunk( X, Y );
	if( ch->GetBlock( x, y, z )->Type() == WATER )
	{

	}
	else if( ch->GetBlock( x, y, z )->Type() == FIRE_STONE )
	{
		ch->DeleteLightSource( x, y, z );
		ch->SetBlockAndTransparency( x, y, z, NormalBlock( AIR ),
									 TRANSPARENCY_AIR );
		RelightBlockAdd( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		UpdateInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
	}
	else
	{
		ch->SetBlockAndTransparency( x, y, z, NormalBlock( AIR ),
									 TRANSPARENCY_AIR );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		//AddFireLight_r( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z, 10 );
		UpdateInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_SUN_LIGHT );
		UpdateWaterInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_SUN_LIGHT );
	}
}

void h_World::FlushActionQueue()
{
	action_queue_mutex_.lock();
	action_queue_[0].swap( action_queue_[1] );
	action_queue_mutex_.unlock();

	while( action_queue_[1].size() != 0 )
	{
		h_WorldAction act= action_queue_[1].front();
		action_queue_[1].pop();

		if( act.type == ACTION_BUILD )
			Build( act.coord[0], act.coord[1], act.coord[2], act.block_type );
		else if( act.type == ACTION_DESTROY )
			Destroy( act.coord[0], act.coord[1], act.coord[2] );
	}
}

void h_World::UpdateInRadius( short x, short y, short r )
{
	r_IWorldRendererPtr renderer= renderer_.lock();
	if( renderer == nullptr )
		return;

	short x_min, x_max, y_min, y_max;
	x_min= ClampX( x - r );
	x_max= ClampX( x + r );
	y_min= ClampY( y - r );
	y_max= ClampY( y + r );

	x_min>>= H_CHUNK_WIDTH_LOG2;
	x_max>>= H_CHUNK_WIDTH_LOG2;
	y_min>>= H_CHUNK_WIDTH_LOG2;
	y_max>>= H_CHUNK_WIDTH_LOG2;
	for( short i= x_min; i<= x_max; i++ )
		for( short j= y_min; j<= y_max; j++ )
			renderer->UpdateChunk( i, j );
}

void h_World::UpdateWaterInRadius( short x, short y, short r )
{
	r_IWorldRendererPtr renderer= renderer_.lock();
	if( renderer == nullptr )
		return;

	short x_min, x_max, y_min, y_max;
	x_min= ClampX( x - r );
	x_max= ClampX( x + r );
	y_min= ClampY( y - r );
	y_max= ClampY( y + r );

	x_min>>= H_CHUNK_WIDTH_LOG2;
	x_max>>= H_CHUNK_WIDTH_LOG2;
	y_min>>= H_CHUNK_WIDTH_LOG2;
	y_max>>= H_CHUNK_WIDTH_LOG2;
	for( short i= x_min; i<= x_max; i++ )
		for( short j= y_min; j<= y_max; j++ )
			renderer->UpdateChunkWater( i, j );
}

void h_World::MoveWorld( h_WorldMoveDirection dir )
{
	int i, j;
	switch ( dir )
	{
	case NORTH:
		for( i= 0; i< chunk_number_x_; i++ )
		{
			h_Chunk* deleted_chunk= chunks_[ i | ( 0 << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader_.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( j= 1; j< chunk_number_y_; j++ )
			{
				chunks_[ i | ( (j-1) << H_MAX_CHUNKS_LOG2 ) ]=
					chunks_[ i | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}

			chunks_[ i | ( (chunk_number_y_-1) << H_MAX_CHUNKS_LOG2 ) ]=
				LoadChunk( i + longitude_, chunk_number_y_ + latitude_ );
		}
		for( i= 0; i< chunk_number_x_; i++ )
			AddLightToBorderChunk( i, chunk_number_y_ - 1 );
		latitude_++;

		break;

	case SOUTH:
		for( i= 0; i< chunk_number_x_; i++ )
		{
			h_Chunk* deleted_chunk= chunks_[ i | ( (chunk_number_y_-1) << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader_.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( j= chunk_number_y_-1; j> 0; j-- )
			{
				chunks_[ i | ( j << H_MAX_CHUNKS_LOG2 ) ]=
					chunks_[ i | ( (j-1) << H_MAX_CHUNKS_LOG2 ) ];
			}

			chunks_[ i | ( 0 << H_MAX_CHUNKS_LOG2 ) ]=
				LoadChunk( i + longitude_,  latitude_-1 );
		}
		for( i= 0; i< chunk_number_x_; i++ )
			AddLightToBorderChunk( i, 0 );
		latitude_--;

		break;

	case EAST:
		for( j= 0; j< chunk_number_y_; j++ )
		{
			h_Chunk* deleted_chunk= chunks_[ 0 | ( j << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader_.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( i= 1; i< chunk_number_x_; i++ )
			{
				chunks_[ (i-1) | ( j << H_MAX_CHUNKS_LOG2 ) ]=
					chunks_[ i | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}
			chunks_[ ( chunk_number_x_-1) | ( j << H_MAX_CHUNKS_LOG2 ) ]=
				LoadChunk( longitude_+chunk_number_x_, latitude_ + j );
		}
		for( j= 0; j< chunk_number_y_; j++ )
			AddLightToBorderChunk( chunk_number_x_-1, j );
		longitude_++;

		break;

	case WEST:
		for( j= 0; j< chunk_number_y_; j++ )
		{
			h_Chunk* deleted_chunk= chunks_[ ( chunk_number_x_-1) | ( j << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader_.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( i= chunk_number_x_-1; i> 0; i-- )
			{
				chunks_[ i | ( j << H_MAX_CHUNKS_LOG2 ) ]=
					chunks_[ (i-1) | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}
			chunks_[ 0 | ( j << H_MAX_CHUNKS_LOG2 ) ]=
				LoadChunk( longitude_-1, latitude_ + j );
		}
		for( j= 0; j< chunk_number_y_; j++ )
			AddLightToBorderChunk( 0, j );
		longitude_--;

		break;
	};

	// Mark for renderer near-border chunks as updated.
	r_IWorldRendererPtr renderer= renderer_.lock();
	if( renderer != nullptr )
	{
		renderer->UpdateWorldPosition( longitude_, latitude_ );

		switch( dir )
		{
		case NORTH:
			for( i= 0; i< chunk_number_x_; i++ )
			{
				renderer->UpdateChunk( i, chunk_number_y_ - 2 );
				renderer->UpdateChunkWater( i, chunk_number_y_ - 2 );
			}
			break;

		case SOUTH:
			for( i= 0; i< chunk_number_x_; i++ )
			{
				renderer->UpdateChunk( i, 1 );
				renderer->UpdateChunkWater( i, 1 );
			}
			break;

		case EAST:
			for( j= 0; j< chunk_number_y_; j++ )
			{
				renderer->UpdateChunk( chunk_number_x_ - 2, j );
				renderer->UpdateChunkWater( chunk_number_x_ - 2, j );
			}
			break;

		case WEST:
			for( j= 0; j< chunk_number_y_; j++ )
			{
				renderer->UpdateChunk( 1, j );
				renderer->UpdateChunkWater( 1, j );
			}
			break;
		};
	}
}

void h_World::SaveChunk( h_Chunk* ch )
{
	/*QByteArray array;
	QDataStream stream( &array, QIODevice::WriteOnly );

	HEXCHUNK_header header;
	//strcpy( header.format_key, "HEXchunk" );
	header.water_block_count= ch->GetWaterList()->Size();
	header.longitude= ch->Longitude();
	header.latitude= ch->Latitude();

	header.Write( stream );

	ch->SaveChunkToFile( stream );

	QByteArray compressed_chunk= qCompress( array );

	char file_name[128];
	sprintf( file_name, "world/ch_%d_%d", ch->Longitude(), ch->Latitude() );
	FILE* f= fopen( file_name, "wb" );
	fwrite( compressed_chunk.constData(), 1, compressed_chunk.size(), f );
	fclose(f);*/

	QByteArray array;
	QDataStream stream( &array, QIODevice::WriteOnly );

	HEXCHUNK_header header;
	//strcpy( header.format_key, "HEXchunk" );
	header.water_block_count= ch->GetWaterList()->Size();
	header.longitude= ch->Longitude();
	header.latitude= ch->Latitude();

	header.Write( stream );
	ch->SaveChunkToFile( stream );

	chunk_loader_.GetChunkData( ch->Longitude(), ch->Latitude() )= qCompress( array );
}

h_Chunk* h_World::LoadChunk( int lon, int lat )
{
	/*char file_name[128];
	sprintf( file_name, "world/ch_%d_%d", lon, lat );
	FILE* f= fopen( file_name, "rb" );
	if( f == NULL )
		return  new h_Chunk( this, lon, lat );

	int file_len;
	fseek( f, 0, SEEK_END );
	file_len= ftell( f );
	fseek( f, 0, SEEK_SET );

	unsigned char* file_data= new unsigned char[ file_len ];
	fread( file_data, 1, file_len, f );
	fclose(f);

	QByteArray uncompressed_chunk= qUncompress( file_data, file_len );
	delete[] file_data;
	QDataStream stream( &uncompressed_chunk, QIODevice::ReadOnly );

	HEXCHUNK_header header;
	header.Read( stream );

	return new h_Chunk( this, &header, stream );*/

	QByteArray& ba= chunk_loader_.GetChunkData( lon, lat );
	if( ba.size() == 0 )
		return new h_Chunk( this, lon, lat );


	QByteArray uncompressed_chunk= qUncompress( ba );
	QDataStream stream( &uncompressed_chunk, QIODevice::ReadOnly );

	HEXCHUNK_header header;
	header.Read( stream );

	return new h_Chunk( this, &header, stream );
}

void h_World::BuildPhysMesh( h_ChunkPhysMesh* phys_mesh, short x_min, short x_max, short y_min, short y_max, short z_min, short z_max )
{
	short x, y, z;
	short x1, y1;
	phys_mesh->block_sides.Resize(0);
	phys_mesh->upper_block_faces.Resize(0);

	short X= Longitude() * H_CHUNK_WIDTH;
	short Y= Latitude() * H_CHUNK_WIDTH;

	x_min= std::max( short(2), x_min );
	y_min= std::max( short(2), y_min );
	z_min= std::max( short(0), z_min );
	x_max= std::min( x_max, short( chunk_number_x_ * H_CHUNK_WIDTH - 2 ) );
	y_max= std::min( y_max, short( chunk_number_y_ * H_CHUNK_WIDTH - 2 ) );
	z_max= std::min( z_max, short( H_CHUNK_HEIGHT - 1 ) );

	p_UpperBlockFace* block_face;
	p_BlockSide* block_side;

	unsigned char t, t_up, t_f, t_fr, t_br;
	for( x= x_min; x< x_max; x++ )
		for( y= y_min; y< y_max; y++ )
		{
			t_up= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )
				  ->Transparency( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z_min );
			for( z= z_min; z < z_max; z++ )
			{
				t= t_up;

				y1= y+1;
				t_f= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1>> H_CHUNK_WIDTH_LOG2 )
					 ->Transparency( x&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

				x1= x+1;
				y1= y + ( 1&(x+1) );
				t_fr= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )
					  ->Transparency( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

				x1= x+1;
				y1= y - (x&1);
				t_br= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )
					  ->Transparency( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

				t_up= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )
					  ->Transparency( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z + 1 );

				if( t != t_up )
				{
					phys_mesh->upper_block_faces.AddToSize(1);
					block_face=  phys_mesh->upper_block_faces.Last();
					if( t > t_up )
						block_face->Gen( x + X, y + Y, z + 1, DOWN );
					else
						block_face->Gen( x + X, y + Y, z, UP );
				}
				if( t != t_fr )
				{
					phys_mesh->block_sides.AddToSize(1);
					block_side= phys_mesh->block_sides.Last();
					if( t > t_fr )
						block_side->Gen( x + X + 1, y + Y + ((x+1)&1), z-1, BACK_LEFT );
					else
						block_side->Gen( x + X, y + Y, z-1, FORWARD_RIGHT );
				}
				if( t != t_br )
				{
					phys_mesh->block_sides.AddToSize(1);
					block_side= phys_mesh->block_sides.Last();
					if( t > t_br )
						block_side->Gen( x + X + 1, y + Y - (x&1), z-1, FORWARD_LEFT );
					else
						block_side->Gen( x + X, y + Y, z-1, BACK_RIGHT );
				}
				if( t!= t_f )
				{
					phys_mesh->block_sides.AddToSize(1);
					block_side= phys_mesh->block_sides.Last();
					if( t > t_f )
						block_side->Gen( x + X, y + Y + 1, z-1, BACK );
					else
						block_side->Gen( x + X, y + Y, z-1, FORWARD );
				}
			}
		}
}

void h_World::BlastBlock_r( short x, short y, short z, short blast_power )
{
	if( blast_power == 0 )
		return;

	h_Chunk* ch= GetChunk( x>>H_CHUNK_WIDTH_LOG2, y>>H_CHUNK_WIDTH_LOG2 );
	unsigned int addr;

	addr= BlockAddr( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
	if( ch->blocks_[addr]->Type() != WATER )
	{
		//ch->SetBlockAndTransparency( local_x, local_y, z, NormalBlock(AIR), TRANSPARENCY_AIR );
		ch->blocks_[ addr ]= NormalBlock(AIR);
		ch->transparency_[ addr ]= TRANSPARENCY_AIR;
	}

	//BlastBlock_r( x, y, z + 1, blast_power-1 );
	//BlastBlock_r( x, y, z - 1, blast_power-1 );
	BlastBlock_r( x, y+1, z, blast_power-1 );
	BlastBlock_r( x, y-1, z, blast_power-1 );
	BlastBlock_r( x+1, y+((x+1)&1), z, blast_power-1 );
	BlastBlock_r( x+1, y-(x&1), z, blast_power-1 );
	BlastBlock_r( x-1, y+((x+1)&1), z, blast_power-1 );
	BlastBlock_r( x-1, y-(x&1), z, blast_power-1 );
}

bool h_World::InBorders( short x, short y, short z ) const
{
	bool outside= x < 0 || y < 0 ||
				  x > H_CHUNK_WIDTH * ChunkNumberX() || y > H_CHUNK_WIDTH * ChunkNumberY() ||
				  z < 0 || z >= H_CHUNK_HEIGHT;
	return !outside;
}

bool h_World::CanBuild( short x, short y, short z ) const
{
	return GetChunk( x>> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
		   GetBlock( x% H_CHUNK_WIDTH, y% H_CHUNK_WIDTH, z )->Type() == AIR;
}

void h_World::PhysTick()
{
	while(!terminated_.load())
	{
		int64_t t0_ms = clock() * 1000 / CLOCKS_PER_SEC;

		h_PlayerPtr player= player_.lock();

		FlushActionQueue();
		WaterPhysTick();
		RelightWaterModifedChunksLight();

		if( player )
		{
			player->Lock();
			player_coord_[2]= short( player->Pos().z );
			GetHexogonCoord( player->Pos().xy(), &player_coord_[0], &player_coord_[1] );
			player->Unlock();
		}

		player_coord_[0]-= Longitude() * H_CHUNK_WIDTH;
		player_coord_[1]-= Latitude() * H_CHUNK_WIDTH;
		BuildPhysMesh( &player_phys_mesh_,
					   player_coord_[0] - 4, player_coord_[0] + 4,
					   player_coord_[1] - 5, player_coord_[1] + 5,
					   player_coord_[2] - 5, player_coord_[2] + 5 );

		if( player_coord_[1]/H_CHUNK_WIDTH > chunk_number_y_/2+2 )
			MoveWorld( NORTH );
		else if( player_coord_[1]/H_CHUNK_WIDTH < chunk_number_y_/2-2 )
			MoveWorld( SOUTH );
		if( player_coord_[0]/H_CHUNK_WIDTH > chunk_number_x_/2+2 )
			MoveWorld( EAST );
		else if( player_coord_[0]/H_CHUNK_WIDTH < chunk_number_x_/2-2 )
			MoveWorld( WEST );

		if( player )
		{
			player->Lock();
			player->SetCollisionMesh( &player_phys_mesh_ );
			player->Unlock();
		}

		phys_tick_count_++;

		if( r_IWorldRendererPtr renderer= renderer_.lock() )
			renderer->Update();

		int64_t t1_ms= clock() * 1000 / CLOCKS_PER_SEC;
		unsigned int dt_ms= (unsigned int)(t1_ms - t0_ms);
		if (dt_ms < 50)
			std::this_thread::sleep_for(std::chrono::milliseconds(50 - dt_ms));
	}
}

void h_World::WaterPhysTick()
{
	h_Chunk* ch;
	for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
		for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
		{
			bool chunk_modifed= false;
			ch= GetChunk( i, j );

			//skip some quadchunks for updating ( in chessboard order )
			if( ( ( ChunkCoordToQuadchunkX( ch->Longitude() ) ^ ChunkCoordToQuadchunkY( ch->Latitude() ) ) & 1 ) == (phys_tick_count_&1) )
				continue;

			auto l= & ch->water_blocks_data.water_block_list;
			h_LiquidBlock* b;
			m_Collection< h_LiquidBlock* >::Iterator iter(l);
			for( iter.Begin(); iter.IsValid(); iter.Next() )
			{
				b= *iter;

				if( ch->GetBlock( b->x_, b->y_, b->z_ - 1 )->Type() == AIR )
				{
					b->z_--;
					ch->SetBlockAndTransparency( b->x_, b->y_, b->z_    , b, TRANSPARENCY_LIQUID );
					ch->SetBlockAndTransparency( b->x_, b->y_, b->z_ + 1, NormalBlock( AIR ), TRANSPARENCY_AIR );
					chunk_modifed= true;
				}
				else
				{
					//water flow down
					h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( b->x_, b->y_, b->z_ - 1 );
					if( b2->Type() == WATER )
					{
						short max_down_cell_water_level;
						if( b->LiquidLevel() >= H_MAX_WATER_LEVEL )
							max_down_cell_water_level= b->LiquidLevel() + H_WATER_COMPRESSION_PER_BLOCK;
						else
						{
							max_down_cell_water_level= H_MAX_WATER_LEVEL + (b->LiquidLevel() *
													   H_WATER_COMPRESSION_PER_BLOCK) /H_MAX_WATER_LEVEL;
						}

						short level_delta= max_down_cell_water_level - b2->LiquidLevel();
						if( level_delta > 0 )
						{
							level_delta= std::min( level_delta, (short) b->LiquidLevel() );
							b->DecreaseLiquidLevel( level_delta );
							b2->IncreaseLiquidLevel( level_delta );
							chunk_modifed= true;
						}
						else if( level_delta < 0 )
						{
							level_delta= std::min( (short)-level_delta, (short)b2->LiquidLevel() );
							b->IncreaseLiquidLevel( level_delta );
							b2->DecreaseLiquidLevel( level_delta );
							chunk_modifed= true;
						}
					}//water flow down

					//water flow up
					b2= (h_LiquidBlock*)ch->GetBlock( b->x_, b->y_, b->z_ + 1 );
					if( b2->Type() == AIR )
					{
						short level_delta= b->LiquidLevel() - H_MAX_WATER_LEVEL;
						if( level_delta > 0  )
						{
							//short level_delta= from->LiquidLevel() / 2;
							b->DecreaseLiquidLevel( level_delta );

							h_LiquidBlock* new_block= ch->NewWaterBlock();
							new_block->x_= b->x_;
							new_block->y_= b->y_;
							new_block->z_= b->z_+1;
							new_block->SetLiquidLevel( level_delta );
							ch->SetBlockAndTransparency( new_block->x_, new_block->y_, new_block->z_, new_block, TRANSPARENCY_LIQUID );
							chunk_modifed= true;
						}
					}//water flow up

					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ),
								   b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) + 1, b->z_ ) )//forward
						chunk_modifed= true;
					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
								   b->y_  + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x_+1)&1), b->z_ ) )//FORWARD_RIGHT
						chunk_modifed= true;
					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
								   b->y_  + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x_+1)&1), b->z_ ) )//FORWARD_LEFT
						chunk_modifed= true;

					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ),
								   b->y_  + ( j << H_CHUNK_WIDTH_LOG2 ) - 1, b->z_ )  )//back
						chunk_modifed= true;
					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
								   b->y_  + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x_&1), b->z_ ) )//BACK_RIGHT
						chunk_modifed= true;
					if( WaterFlow( b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
								   b->y_  + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x_&1), b->z_ ) )//BACK_LEFT
						chunk_modifed= true;

					if( b->LiquidLevel() == 0 ||
							(  b->LiquidLevel() < 16 && ch->GetBlock( b->x_, b->y_, b->z_-1 )->Type() != WATER ) )
					{
						iter.RemoveCurrent();
						ch->SetBlockAndTransparency( b->x_,
													 b->y_, b->z_, NormalBlock( AIR ), TRANSPARENCY_AIR );
						ch->DeleteWaterBlock( b );
					}
				}// if down block not air
			}//for all water blocks in chunk
			if( chunk_modifed )
			{
				if( r_IWorldRendererPtr renderer= renderer_.lock() )
				{
					renderer->UpdateChunk( i, j );
					if( i > 0 )
						renderer->UpdateChunkWater( i-1, j );
					if( i < ChunkNumberX() - 1 );
					renderer->UpdateChunkWater( i+1, j );
					if( j > 0 )
						renderer->UpdateChunkWater( i, j-1 );
					if( j < ChunkNumberY() - 1 );
					renderer->UpdateChunkWater( i, j+1 );
				}

				ch->need_update_light_= true;
			}
		}//for chunks
}

bool h_World::WaterFlow( h_LiquidBlock* from, short to_x, short to_y, short to_z )
{
	h_Chunk* ch= GetChunk( to_x >> H_CHUNK_WIDTH_LOG2, to_y >> H_CHUNK_WIDTH_LOG2 );
	h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( to_x & ( H_CHUNK_WIDTH-1 ), to_y & ( H_CHUNK_WIDTH-1 ), to_z );
	unsigned char type= b2->Type();
	if( type == AIR )
	{
		if( from->LiquidLevel() > 1 )
		{
			short level_delta= from->LiquidLevel() / 2;
			from->DecreaseLiquidLevel( level_delta );

			h_LiquidBlock* new_block= ch->NewWaterBlock();
			new_block->x_= to_x & ( H_CHUNK_WIDTH-1 );
			new_block->y_= to_y & ( H_CHUNK_WIDTH-1 );
			new_block->z_= to_z;
			new_block->SetLiquidLevel( level_delta );
			ch->SetBlockAndTransparency( new_block->x_, new_block->y_, new_block->z_, new_block, TRANSPARENCY_LIQUID );
			return true;
		}
	}
	else if( type == WATER )
	{
		short water_level_delta= from->LiquidLevel() - b2->LiquidLevel();
		if( water_level_delta  > 1 )
		{
			water_level_delta/= 2;
			from->DecreaseLiquidLevel( water_level_delta );
			b2->IncreaseLiquidLevel( water_level_delta );
			return true;
		}
	}
	return false;
}

void h_World::InitNormalBlocks()
{
	int i;
	for( i= 0; i< NUM_BLOCK_TYPES; i++ )
		new ( normal_blocks_ + i ) h_Block( h_BlockType(i) );
}
