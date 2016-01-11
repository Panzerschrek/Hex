#include <chrono>

#include "world.hpp"
#include "player.hpp"
#include "math_lib/math.hpp"
#include "renderer/i_world_renderer.hpp"
#include "settings.hpp"
#include "settings_keys.hpp"
#include "world_generator/world_generator.hpp"
#include "world_header.hpp"
#include "chunk_phys_mesh.hpp"
#include "path_finder.hpp"
#include "console.hpp"
#include "ticks_counter.hpp"

static constexpr const unsigned int g_updates_frequency= 15;
static constexpr const unsigned int g_update_inrerval_ms= 1000 / g_updates_frequency;

static constexpr const unsigned int g_day_duration_ticks= 12 /*min*/ * 60 /*sec*/ * g_updates_frequency;
static constexpr const unsigned int g_days_in_year= 32;
static constexpr const unsigned int g_northern_hemisphere_summer_solstice_day= g_days_in_year / 4;
static constexpr const float g_planet_rotation_axis_inclination= 23.439281f * m_Math::deg2rad;
static constexpr const float g_global_world_latitude= 40.0f * m_Math::deg2rad;

// day of spring equinox
// some time after sunrise.
static constexpr const unsigned int g_world_start_tick=
	( g_days_in_year + g_northern_hemisphere_summer_solstice_day - g_days_in_year / 4 ) %
	g_days_in_year *
	g_day_duration_ticks +
	g_day_duration_ticks / 4 + g_day_duration_ticks / 16;

h_World::h_World(
	const h_SettingsPtr& settings,
	const h_WorldHeaderPtr& header,
	const char* world_directory )
	: settings_( settings )
	, header_( header )
	, chunk_loader_( world_directory )
	, calendar_(
		g_day_duration_ticks,
		g_days_in_year,
		g_planet_rotation_axis_inclination,
		g_northern_hemisphere_summer_solstice_day )
	, phys_tick_count_( header->ticks != 0 ? header->ticks : g_world_start_tick )
{
	InitNormalBlocks();

	chunk_number_x_= std::max( std::min( settings_->GetInt( h_SettingsKeys::chunk_number_x, 14 ), H_MAX_CHUNKS ), H_MIN_CHUNKS );
	chunk_number_y_= std::max( std::min( settings_->GetInt( h_SettingsKeys::chunk_number_y, 12 ), H_MAX_CHUNKS ), H_MIN_CHUNKS );

	// Active area margins. Minimal active area have size 5.
	active_area_margins_[0]=
		std::max(
			2,
			std::min(
				settings_->GetInt( h_SettingsKeys::active_area_margins_x, 2 ),
				int(chunk_number_x_ / 2 - 2) ) );
	active_area_margins_[1]=
		std::max(
			2,
			std::min(
				settings_->GetInt( h_SettingsKeys::active_area_margins_y, 2 ),
				int(chunk_number_y_ / 2 - 2) ) );

	settings_->SetSetting( h_SettingsKeys::chunk_number_x, (int)chunk_number_x_ );
	settings_->SetSetting( h_SettingsKeys::chunk_number_y, (int)chunk_number_y_ );
	settings_->SetSetting( h_SettingsKeys::active_area_margins_x, (int)active_area_margins_[0] );
	settings_->SetSetting( h_SettingsKeys::active_area_margins_y, (int)active_area_margins_[1] );

	{ // Move world to player position
		short player_xy[2];
		GetHexogonCoord( m_Vec2(header_->player.x, header->player.y), &player_xy[0], &player_xy[1] );
		int player_longitude= ( player_xy[0] + (H_CHUNK_WIDTH >> 1) ) >> H_CHUNK_WIDTH_LOG2;
		int player_latitude = ( player_xy[1] + (H_CHUNK_WIDTH >> 1) ) >> H_CHUNK_WIDTH_LOG2;

		longitude_= player_longitude - chunk_number_x_/2;
		latitude_ = player_latitude  - chunk_number_y_/2;
	}

	g_WorldGenerationParameters parameters;
	parameters.world_dir= world_directory;
	parameters.size[0]= parameters.size[1]= 512;
	parameters.cell_size_log2= 0;
	parameters.seed= 24;

	world_generator_.reset( new g_WorldGenerator( parameters ) );
	world_generator_->Generate();
	//world_generator_->DumpDebugResult();

	for( unsigned int i= 0; i< chunk_number_x_; i++ )
	for( unsigned int j= 0; j< chunk_number_y_; j++ )
		chunks_[ i + j * H_MAX_CHUNKS ]= LoadChunk( i+longitude_, j+latitude_);

	LightWorld();

	test_mob_target_pos_[0]= test_mob_discret_pos_[0]= 0;
	test_mob_target_pos_[1]= test_mob_discret_pos_[1]= 0;
	test_mob_target_pos_[2]= test_mob_discret_pos_[2]= 72;
}

h_World::~h_World()
{
	header_->ticks= phys_tick_count_;

	H_ASSERT(!phys_thread_);

	for( unsigned int x= 0; x< chunk_number_x_; x++ )
		for( unsigned int y= 0; y< chunk_number_y_; y++ )
		{
			h_Chunk* ch= GetChunk(x,y);
			SaveChunk( ch );
			chunk_loader_.FreeChunkData( ch->Longitude(), ch->Latitude() );
			delete ch;
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

void h_World::StartUpdates( h_Player* player, r_IWorldRenderer* renderer )
{
	H_ASSERT( player );
	H_ASSERT( renderer );
	H_ASSERT( !player_ );
	H_ASSERT( !renderer_ );
	H_ASSERT( !phys_thread_ );

	player_= player;
	renderer_= renderer;

	phys_thread_need_stop_.store(false);
	phys_thread_.reset( new std::thread( &h_World::PhysTick, this ) );

	h_Console::Info( "World updates started" );
}

void h_World::StopUpdates()
{
	H_ASSERT( phys_thread_ );

	H_ASSERT( player_ );
	H_ASSERT( renderer_ );
	player_= nullptr;
	renderer_= nullptr;

	phys_thread_need_stop_.store(true);
	phys_thread_->join();
	phys_thread_.reset();

	h_Console::Info( "World updates stopped" );
}

void h_World::Save()
{
	for( unsigned int x= 0; x< chunk_number_x_; x++ )
		for( unsigned int y= 0; y< chunk_number_y_; y++ )
			SaveChunk( GetChunk(x,y) );
	chunk_loader_.ForceSaveAllChunks();
}

unsigned int h_World::GetTimeOfYear() const
{
	return phys_tick_count_ % ( g_day_duration_ticks * g_days_in_year );
}

const h_Calendar& h_World::GetCalendar() const
{
	return calendar_;
}

float h_World::GetGlobalWorldLatitude() const
{
	return g_global_world_latitude;
}

void h_World::TestMobSetTargetPosition( int x, int y, int z )
{
	test_mob_target_pos_[0]= x;
	test_mob_target_pos_[1]= y;
	test_mob_target_pos_[2]= z;
}

const m_Vec3& h_World::TestMobGetPosition() const
{
	return test_mob_pos_;
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
	if( block_type == h_BlockType::Water )
	{
		h_LiquidBlock* b;
		h_Chunk* ch=GetChunk( X, Y );
		ch-> SetBlockAndTransparency(
			x, y, z,
			b= ch->NewWaterBlock(), TRANSPARENCY_LIQUID );

		b->x_= x;
		b->y_= y;
		b->z_= z;
	}
	else if( block_type == h_BlockType::FireStone )
	{
		h_Chunk* ch=GetChunk( X, Y );
		h_LightSource* s= ch->NewLightSource( x, y, z, h_BlockType::FireStone );
		s->SetLightLevel( H_MAX_FIRE_LIGHT );
		ch->SetBlockAndTransparency( x, y, z, s, TRANSPARENCY_SOLID );
		AddFireLight_r( x + X* H_CHUNK_WIDTH, y + Y* H_CHUNK_WIDTH, z, H_MAX_FIRE_LIGHT );
	}
	else
		GetChunk( X, Y )->
		SetBlockAndTransparency(
			x, y, z,
			NormalBlock( block_type ),
			NormalBlock( block_type )->Transparency() );

	short r= 1;
	if( block_type != h_BlockType::Water )
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
	if( ch->GetBlock( x, y, z )->Type() == h_BlockType::Water )
	{

	}
	else if( ch->GetBlock( x, y, z )->Type() == h_BlockType::FireStone )
	{
		ch->DeleteLightSource( x, y, z );
		ch->SetBlockAndTransparency(
			x, y, z,
			NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );
		RelightBlockAdd( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		UpdateInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
	}
	else
	{
		ch->SetBlockAndTransparency(
			x, y, z,
			NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
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
			renderer_->UpdateChunk( i, j );
}

void h_World::UpdateWaterInRadius( short x, short y, short r )
{
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
			renderer_->UpdateChunkWater( i, j );
}

void h_World::MoveWorld( h_WorldMoveDirection dir )
{
	unsigned int i, j;
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
	renderer_->UpdateWorldPosition( longitude_, latitude_ );

	switch( dir )
	{
	case NORTH:
		for( i= 0; i< chunk_number_x_; i++ )
		{
			renderer_->UpdateChunk( i, chunk_number_y_ - 2, true );
			renderer_->UpdateChunkWater( i, chunk_number_y_ - 2, true );
		}
		break;

	case SOUTH:
		for( i= 0; i< chunk_number_x_; i++ )
		{
			renderer_->UpdateChunk( i, 1, true );
			renderer_->UpdateChunkWater( i, 1, true );
		}
		break;

	case EAST:
		for( j= 0; j< chunk_number_y_; j++ )
		{
			renderer_->UpdateChunk( chunk_number_x_ - 2, j, true );
			renderer_->UpdateChunkWater( chunk_number_x_ - 2, j, true );
		}
		break;

	case WEST:
		for( j= 0; j< chunk_number_y_; j++ )
		{
			renderer_->UpdateChunk( 1, j, true );
			renderer_->UpdateChunkWater( 1, j, true );
		}
		break;
	};
}

void h_World::SaveChunk( h_Chunk* ch )
{
	QByteArray array;
	QDataStream stream( &array, QIODevice::WriteOnly );

	HEXCHUNK_header header;

	header.water_block_count= ch->GetWaterList()->Size();
	header.longitude= ch->Longitude();
	header.latitude= ch->Latitude();

	header.Write( stream );
	ch->SaveChunkToFile( stream );

	chunk_loader_.GetChunkData( ch->Longitude(), ch->Latitude() )= qCompress( array );
}

h_Chunk* h_World::LoadChunk( int lon, int lat )
{
	QByteArray& ba= chunk_loader_.GetChunkData( lon, lat );
	if( ba.size() == 0 )
		return new h_Chunk( this, lon, lat, world_generator_.get() );

	QByteArray uncompressed_chunk= qUncompress( ba );
	QDataStream stream( &uncompressed_chunk, QIODevice::ReadOnly );

	HEXCHUNK_header header;
	header.Read( stream );

	return new h_Chunk( this, header, stream );
}

h_ChunkPhysMesh h_World::BuildPhysMesh( short x_min, short x_max, short y_min, short y_max, short z_min, short z_max )
{
	h_ChunkPhysMesh phys_mesh;

	short X= Longitude() * H_CHUNK_WIDTH;
	short Y= Latitude () * H_CHUNK_WIDTH;

	x_min= std::max( short(2), x_min );
	y_min= std::max( short(2), y_min );
	z_min= std::max( short(0), z_min );
	x_max= std::min( x_max, short( chunk_number_x_ * H_CHUNK_WIDTH - 2 ) );
	y_max= std::min( y_max, short( chunk_number_y_ * H_CHUNK_WIDTH - 2 ) );
	z_max= std::min( z_max, short( H_CHUNK_HEIGHT - 1 ) );

	for( short x= x_min; x< x_max; x++ )
	for( short y= y_min; y< y_max; y++ )
	{
		const unsigned char *t_p, *t_f_p, *t_fr_p, *t_br_p;
		short x1, y1;

		t_p=
			GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 )->GetTransparencyData() +
			BlockAddr( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), 0 );

		y1= y + 1;
		t_f_p=
			GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )->GetTransparencyData() +
			BlockAddr( x&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), 0 );

		x1= x+1;
		y1= y + ( 1&(x+1) );
		t_fr_p=
			GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )->GetTransparencyData() +
			BlockAddr( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), 0 );

		x1= x+1;
		y1= y - (x&1);
		t_br_p=
			GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )->GetTransparencyData() +
			BlockAddr( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), 0 );

		for( short z= z_min; z < z_max; z++ )
		{
			unsigned char t, t_up, t_f, t_fr, t_br;
			t= t_p[z];
			t_up= t_p[z+1];
			t_f= t_f_p[z];
			t_fr= t_fr_p[z];
			t_br= t_br_p[z];

			if( t != t_up )
			{
				if( t > t_up )
					phys_mesh.upper_block_faces.emplace_back( x + X, y + Y, z + 1, h_Direction::Down );
				else
					phys_mesh.upper_block_faces.emplace_back( x + X, y + Y, z, h_Direction::Up );
			}
			if( t != t_fr )
			{
				if( t > t_fr )
					phys_mesh.block_sides.emplace_back( x + X + 1, y + Y + ((x+1)&1), z-1, h_Direction::BackLeft );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z-1, h_Direction::ForwardRight );
			}
			if( t != t_br )
			{
				if( t > t_br )
					phys_mesh.block_sides.emplace_back( x + X + 1, y + Y - (x&1), z-1, h_Direction::ForwardLeft );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z-1, h_Direction::BackRight );
			}
			if( t!= t_f )
			{
				if( t > t_f )
					phys_mesh.block_sides.emplace_back( x + X, y + Y + 1, z-1, h_Direction::Back );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z-1, h_Direction::Forward );
			}
		} // for z
	} // for xy

	return phys_mesh;
}

void h_World::BlastBlock_r( short x, short y, short z, short blast_power )
{
	if( blast_power == 0 )
		return;

	h_Chunk* ch= GetChunk( x>>H_CHUNK_WIDTH_LOG2, y>>H_CHUNK_WIDTH_LOG2 );
	unsigned int addr;

	addr= BlockAddr( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
	if( ch->blocks_[addr]->Type() != h_BlockType::Water )
	{
		//ch->SetBlockAndTransparency( local_x, local_y, z, NormalBlock(AIR), TRANSPARENCY_AIR );
		ch->blocks_[ addr ]= NormalBlock(h_BlockType::Air);
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
	bool outside=
		x < 0 || y < 0 ||
		x > int(H_CHUNK_WIDTH * ChunkNumberX()) || y > int(H_CHUNK_WIDTH * ChunkNumberY()) ||
		z < 0 || z >= H_CHUNK_HEIGHT;
	return !outside;
}

bool h_World::CanBuild( short x, short y, short z ) const
{
	return
		GetChunk( x>> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
		GetBlock( x & (H_CHUNK_WIDTH - 1), y & (H_CHUNK_WIDTH - 1), z )->Type() == h_BlockType::Air;
}

void h_World::PhysTick()
{
	while(!phys_thread_need_stop_.load())
	{
		H_ASSERT( player_ );
		H_ASSERT( renderer_ );

		TestMobTick();

		uint64_t t0_ms = hGetTimeMS();

		FlushActionQueue();
		WaterPhysTick();
		RelightWaterModifedChunksLight();

		// player logic
		{
			player_->Lock();
			short player_coord_global[2];
			GetHexogonCoord( player_->Pos().xy(), &player_coord_global[0], &player_coord_global[1] );
			player_->Unlock();

			int player_coord[3];
			player_coord[0]= player_coord_global[0] - Longitude() * H_CHUNK_WIDTH;
			player_coord[1]= player_coord_global[1] - Latitude () * H_CHUNK_WIDTH;
			player_coord[2]= int(player_->Pos().z + H_PLAYER_EYE_LEVEL);
			h_ChunkPhysMesh player_phys_mesh=
				BuildPhysMesh(
					player_coord[0] - 5, player_coord[0] + 5,
					player_coord[1] - 6, player_coord[1] + 6,
					player_coord[2] - 5, player_coord[2] + 5 );

			int player_chunk_x= ( player_coord[0] + (H_CHUNK_WIDTH>>1) ) >> H_CHUNK_WIDTH_LOG2;
			int player_chunk_y= ( player_coord[1] + (H_CHUNK_WIDTH>>1) ) >> H_CHUNK_WIDTH_LOG2;

			if( player_chunk_y > int(chunk_number_y_/2+2) )
				MoveWorld( NORTH );
			else if( player_chunk_y < int(chunk_number_y_/2-2) )
				MoveWorld( SOUTH );
			if( player_chunk_x > int(chunk_number_x_/2+2) )
				MoveWorld( EAST );
			else if( player_chunk_x < int(chunk_number_x_/2-2) )
				MoveWorld( WEST );

			player_->Lock();
			player_->SetCollisionMesh( std::move(player_phys_mesh) );
			player_->Unlock();
		}

		phys_tick_count_++;

		renderer_->Update();

		uint64_t t1_ms= hGetTimeMS();
		unsigned int dt_ms= (unsigned int)(t1_ms - t0_ms);
		if (dt_ms < g_update_inrerval_ms)
			std::this_thread::sleep_for(std::chrono::milliseconds(g_update_inrerval_ms - dt_ms));
	}
}

void h_World::TestMobTick()
{
	if( phys_tick_count_ - test_mob_last_think_tick_ >= g_updates_frequency / 3u )
	{
		test_mob_last_think_tick_= phys_tick_count_;

		if( test_mob_discret_pos_[0] != test_mob_target_pos_[0] ||
			test_mob_discret_pos_[1] != test_mob_target_pos_[1] ||
			test_mob_discret_pos_[2] != test_mob_target_pos_[2] )
		{
			h_PathFinder finder(*this);
			if( finder.FindPath(
				test_mob_discret_pos_[0], test_mob_discret_pos_[1], test_mob_discret_pos_[2],
				test_mob_target_pos_[0], test_mob_target_pos_[1], test_mob_target_pos_[2] ) )
			{
				const h_PathPoint* path= finder.GetPathPoints() + finder.GetPathLength() - 1;
				test_mob_discret_pos_[0]= path->x;
				test_mob_discret_pos_[1]= path->y;
				test_mob_discret_pos_[2]= path->z;
			}
		}
	}

	test_mob_pos_.x= ( float(test_mob_discret_pos_[0]) + 1.0f / 3.0f ) * H_SPACE_SCALE_VECTOR_X;
	test_mob_pos_.y= float(test_mob_discret_pos_[1]) + 0.5f * float((test_mob_discret_pos_[0]^1)&1);
	test_mob_pos_.z= float(test_mob_discret_pos_[2]);
}

void h_World::WaterPhysTick()
{
	h_Chunk* ch;
	for( unsigned int i= active_area_margins_[0]; i< ChunkNumberX() - active_area_margins_[0]; i++ )
		for( unsigned int j= active_area_margins_[1]; j< ChunkNumberY() - active_area_margins_[1]; j++ )
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

				if( ch->GetBlock( b->x_, b->y_, b->z_ - 1 )->Type() == h_BlockType::Air )
				{
					b->z_--;
					ch->SetBlockAndTransparency( b->x_, b->y_, b->z_    , b, TRANSPARENCY_LIQUID );
					ch->SetBlockAndTransparency( b->x_, b->y_, b->z_ + 1, NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );
					chunk_modifed= true;
				}
				else
				{
					//water flow down
					h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( b->x_, b->y_, b->z_ - 1 );
					if( b2->Type() == h_BlockType::Water )
					{
						int level_delta= std::min(int(H_MAX_WATER_LEVEL - b2->LiquidLevel()), int(b->LiquidLevel()));
						if( level_delta > 0 )
						{
							b->DecreaseLiquidLevel( level_delta );
							b2->IncreaseLiquidLevel( level_delta );
							chunk_modifed= true;
						}
					} //water flow down


					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ),
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) + 1, b->z_ ) )//forward
						chunk_modifed= true;
					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x_+1)&1), b->z_ ) )//FORWARD_RIGHT
						chunk_modifed= true;
					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x_+1)&1), b->z_ ) )//FORWARD_LEFT
						chunk_modifed= true;

					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ),
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) - 1, b->z_ ) )//back
						chunk_modifed= true;
					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x_&1), b->z_ ) )//BACK_RIGHT
						chunk_modifed= true;
					if( WaterFlow(
						b, b->x_ + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
						b->y_ + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x_&1), b->z_ ) )//BACK_LEFT
						chunk_modifed= true;

					if( b->LiquidLevel() == 0 ||
						( b->LiquidLevel() < 16 && ch->GetBlock( b->x_, b->y_, b->z_-1 )->Type() != h_BlockType::Water ) )
					{
						iter.RemoveCurrent();
						ch->SetBlockAndTransparency( b->x_, b->y_, b->z_, NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );
						ch->DeleteWaterBlock( b );
					}
				}// if down block not air
			}//for all water blocks in chunk
			if( chunk_modifed )
			{
				renderer_->UpdateChunkWater( i  , j   );
				renderer_->UpdateChunkWater( i-1, j   );
				renderer_->UpdateChunkWater( i+1, j   );
				renderer_->UpdateChunkWater( i  , j-1 );
				renderer_->UpdateChunkWater( i  , j+1 );

				ch->need_update_light_= true;
			}
		}//for chunks
}

bool h_World::WaterFlow( h_LiquidBlock* from, short to_x, short to_y, short to_z )
{
	h_Chunk* ch= GetChunk( to_x >> H_CHUNK_WIDTH_LOG2, to_y >> H_CHUNK_WIDTH_LOG2 );
	h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( to_x & ( H_CHUNK_WIDTH-1 ), to_y & ( H_CHUNK_WIDTH-1 ), to_z );
	h_BlockType type= b2->Type();
	if( type == h_BlockType::Air )
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
	else if( type == h_BlockType::Water )
	{
		short water_level_delta= from->LiquidLevel() - b2->LiquidLevel();
		if( water_level_delta > 1 )
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
	for( size_t i= 0; i < size_t(h_BlockType::NumBlockTypes); i++ )
		new ( normal_blocks_ + i ) h_Block( h_BlockType( static_cast<h_BlockType>(i) ) );
}
