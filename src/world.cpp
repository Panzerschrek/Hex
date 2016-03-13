#include <chrono>

#include "world.hpp"
#include "player.hpp"
#include "math_lib/math.hpp"
#include "renderer/i_world_renderer.hpp"
#include "settings.hpp"
#include "settings_keys.hpp"
#include "world_generator/world_generator.hpp"
#include "world_header.hpp"
#include "world_phys_mesh.hpp"
#include "path_finder.hpp"
#include "console.hpp"
#include "time.hpp"

static constexpr const unsigned int g_updates_frequency= 15;
static constexpr const unsigned int g_update_inrerval_ms= 1000 / g_updates_frequency;
static constexpr const unsigned int g_sleep_interval_on_pause= g_update_inrerval_ms * 4;

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
	const h_LongLoadingCallback& long_loading_callback,
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
	, phys_thread_need_stop_(false)
	, phys_thread_paused_(false)
	, unactive_grass_block_( 0, 0, 1, false )
{
	const float c_initial_progress= 0.05f;
	const float c_progress_for_generation= 0.2f;
	const float c_progres_per_chunk= 0.01f;
	const float c_lighting_progress= 0.2f;

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
		pGetHexogonCoord( m_Vec2(header_->player.x, header->player.y), &player_xy[0], &player_xy[1] );
		int player_longitude= ( player_xy[0] + (H_CHUNK_WIDTH >> 1) ) >> H_CHUNK_WIDTH_LOG2;
		int player_latitude = ( player_xy[1] + (H_CHUNK_WIDTH >> 1) ) >> H_CHUNK_WIDTH_LOG2;

		longitude_= player_longitude - chunk_number_x_/2;
		latitude_ = player_latitude  - chunk_number_y_/2;
	}

	float progress_scaler= 1.0f / (
		c_initial_progress + c_progress_for_generation +
		c_progres_per_chunk * float( chunk_number_x_ * chunk_number_y_ ) +
		c_lighting_progress );
	float progress= 0.0f;

	long_loading_callback( progress+= c_initial_progress * progress_scaler );

	g_WorldGenerationParameters parameters;
	parameters.world_dir= world_directory;
	parameters.size[0]= parameters.size[1]= 512;
	parameters.cell_size_log2= 0;
	parameters.seed= 24;

	world_generator_.reset( new g_WorldGenerator( parameters ) );
	world_generator_->Generate();
	//world_generator_->DumpDebugResult();

	long_loading_callback( progress+= c_progress_for_generation * progress_scaler );

	for( unsigned int i= 0; i< chunk_number_x_; i++ )
	for( unsigned int j= 0; j< chunk_number_y_; j++ )
	{
		chunks_[ i + j * H_MAX_CHUNKS ]= LoadChunk( i+longitude_, j+latitude_);

		long_loading_callback( progress+= c_progres_per_chunk * progress_scaler );
	}

	LightWorld();

	long_loading_callback( progress+= c_lighting_progress * progress_scaler );

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
	h_WorldAction act;
	act.type= h_WorldAction::Type::Build;
	act.block_type= block_type;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	std::lock_guard< std::mutex > lock(action_queue_mutex_);
	action_queue_[0].push( act );
}

void h_World::AddDestroyEvent( short x, short y, short z )
{
	h_WorldAction act;
	act.type= h_WorldAction::Type::Destroy;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	std::lock_guard< std::mutex > lock(action_queue_mutex_);
	action_queue_[0].push( act );
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
	phys_thread_paused_.store(false);
	phys_thread_.reset( new std::thread( &h_World::PhysTick, this ) );

	h_Console::Info( "World updates started" );
}

void h_World::StopUpdates()
{
	H_ASSERT( phys_thread_ );

	H_ASSERT( player_ );
	H_ASSERT( renderer_ );

	phys_thread_need_stop_.store(true);
	phys_thread_paused_.store(false);
	phys_thread_->join();
	phys_thread_.reset();

	player_= nullptr;
	renderer_= nullptr;

	h_Console::Info( "World updates stopped" );
}

void h_World::PauseUpdates()
{
	H_ASSERT( phys_thread_ );

	phys_thread_paused_.store(true);
}

void h_World::UnpauseUpdates()
{
	H_ASSERT( phys_thread_ );

	phys_thread_paused_.store(false);
}

p_WorldPhysMeshConstPtr h_World::GetPhysMesh() const
{
	std::lock_guard<std::mutex> lock( phys_mesh_mutex_ );
	return phys_mesh_;
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

	int local_x= x & (H_CHUNK_WIDTH - 1);
	int local_y= y & (H_CHUNK_WIDTH - 1);

	int chunk_x= x >> H_CHUNK_WIDTH_LOG2;
	int chunk_y= y >> H_CHUNK_WIDTH_LOG2;

	if( block_type == h_BlockType::Water )
	{
		h_LiquidBlock* b;
		h_Chunk* ch= GetChunk( chunk_x, chunk_y );
		ch-> SetBlockAndTransparency(
			local_x, local_y, z,
			b= ch->NewWaterBlock(), TRANSPARENCY_LIQUID );

		b->x_= local_x;
		b->y_= local_y;
		b->z_= z;
	}
	else if( block_type == h_BlockType::FireStone )
	{
		h_Chunk* ch= GetChunk( chunk_x, chunk_y );
		h_LightSource* s= ch->NewLightSource( local_x, local_y, z, h_BlockType::FireStone );
		s->SetLightLevel( H_MAX_FIRE_LIGHT );

		ch->SetBlockAndTransparency( local_x, local_y, z, s, TRANSPARENCY_SOLID );
		AddFireLight_r( x, y, z, H_MAX_FIRE_LIGHT );
	}
	else if( block_type == h_BlockType::Grass )
	{
		h_Chunk* ch= GetChunk( chunk_x, chunk_y );
		h_GrassBlock* grass_block= ch->NewActiveGrassBlock( local_x, local_y, z );
		ch->SetBlockAndTransparency( local_x, local_y, z, grass_block, TRANSPARENCY_SOLID );
	}
	else
		GetChunk( chunk_x, chunk_y )->
		SetBlockAndTransparency(
			local_x, local_y, z,
			NormalBlock( block_type ),
			NormalBlock( block_type )->Transparency() );

	short r= 1;
	if( block_type != h_BlockType::Water )
		r= RelightBlockAdd( x, y, z ) + 1;

	UpdateInRadius( x, y, r );
	UpdateWaterInRadius( x, y, r );

	CheckBlockNeighbors( x, y, z );
}

void h_World::Destroy( short x, short y, short z )
{
	if( !InBorders( x, y, z ) )
		return;

	int local_x= x & (H_CHUNK_WIDTH - 1);
	int local_y= y & (H_CHUNK_WIDTH - 1);

	int chunk_x= x >> H_CHUNK_WIDTH_LOG2;
	int chunk_y= y >> H_CHUNK_WIDTH_LOG2;

	h_Chunk* ch= GetChunk( chunk_x, chunk_y );
	h_Block* block= ch->GetBlock( local_x, local_y, z );
	if( block->Type() == h_BlockType::Water )
	{

	}
	else if( block->Type() == h_BlockType::FireStone )
	{
		ch->DeleteLightSource( local_x, local_y, z );
		ch->SetBlockAndTransparency(
			local_x, local_y, z,
			NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );

		RelightBlockAdd( x, y, z );

		RelightBlockRemove( x, y, z );
		UpdateInRadius( x, y, H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x, y, H_MAX_FIRE_LIGHT );
	}
	else if( block->Type() == h_BlockType::Grass )
	{
		// Delete grass block from list of active grass blocks, if it is active.
		h_GrassBlock* grass_block= static_cast<h_GrassBlock*>(block);
		if( grass_block->IsActive() )
		{
			bool deleted= false;
			for( unsigned int i= 0; i < ch->active_grass_blocks_.size(); i++ )
			{
				if( ch->active_grass_blocks_[i] == grass_block )
				{
					ch->active_grass_blocks_allocator_.Delete( grass_block );

					if( i != ch->active_grass_blocks_.size() - 1 )
						ch->active_grass_blocks_[i]= ch->active_grass_blocks_.back();

					ch->active_grass_blocks_.pop_back();

					deleted= true;
					break;
				}
			}

			(void)deleted;
			H_ASSERT(deleted);
		}

		ch->SetBlockAndTransparency(
			local_x, local_y, z,
			NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );

		RelightBlockRemove( x, y, z );
		UpdateInRadius( x, y, H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x, y, H_MAX_FIRE_LIGHT );
	}
	else
	{
		ch->SetBlockAndTransparency(
			local_x, local_y, z,
			NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );

		RelightBlockRemove( x, y, z );
		UpdateInRadius( x, y, H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x, y, H_MAX_FIRE_LIGHT );
	}

	CheckBlockNeighbors( x, y, z );
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

		switch( act.type )
		{
		case h_WorldAction::Type::Build:
			Build( act.coord[0], act.coord[1], act.coord[2], act.block_type );
			break;

		case h_WorldAction::Type::Destroy:
			Destroy( act.coord[0], act.coord[1], act.coord[2] );
			break;
		};
	}
}

void h_World::CheckBlockNeighbors( short x, short y, short z )
{
	int forward_side_y= y + ( (x^1) & 1 );
	int back_side_y= y - (x & 1);

	int neighbors[7][2]=
	{
		{ x, y },
		{ x, y + 1 },
		{ x, y - 1 },
		{ x + 1, forward_side_y },
		{ x + 1, back_side_y },
		{ x - 1, forward_side_y },
		{ x - 1, back_side_y },
	};

	for( unsigned int n= 0; n < 7; n++ )
	{
		int chunk_x= neighbors[n][0] >> H_CHUNK_WIDTH_LOG2;
		int chunk_y= neighbors[n][1] >> H_CHUNK_WIDTH_LOG2;

		h_Chunk* chunk= GetChunk( chunk_x, chunk_y );

		int local_x= neighbors[n][0] & (H_CHUNK_WIDTH-1);
		int local_y= neighbors[n][1] & (H_CHUNK_WIDTH-1);
		int neighbor_addr= BlockAddr( local_x, local_y, 0 );

		for( int neighbor_z= std::max(0, int(z) - 2);
			 neighbor_z <= std::min(int(z) + 1, H_CHUNK_HEIGHT - 1);
			 neighbor_z++ )
		{
			h_Block* block= chunk->blocks_[ neighbor_addr + neighbor_z ];
			switch( block->Type() )
			{
				// Activate unactive grass blocks.
				case h_BlockType::Grass:
				{
					h_GrassBlock* grass_block= static_cast<h_GrassBlock*>(block);
					if( !grass_block->IsActive() )
						chunk->blocks_[ neighbor_addr + neighbor_z ]=
							chunk->NewActiveGrassBlock( local_x, local_y, neighbor_z );
				}
				break;

				// If there is air under sand block - sand must fail.
				case h_BlockType::Sand:
				{
					if( chunk->blocks_[ neighbor_addr + neighbor_z - 1 ]->Type() == h_BlockType::Air )
					{
						h_FailingBlock* failing_block= chunk->failing_blocks_alocatior_.New( block, local_x, local_y, neighbor_z );
						chunk->failing_blocks_.push_back( failing_block );
						chunk->SetBlockAndTransparency( local_x, local_y, neighbor_z, failing_block, TRANSPARENCY_AIR );

						RelightBlockRemove( neighbors[n][0], neighbors[n][1], neighbor_z );
						UpdateInRadius( neighbors[n][0], neighbors[n][1], H_MAX_FIRE_LIGHT );
						UpdateWaterInRadius( neighbors[n][0], neighbors[n][1], H_MAX_FIRE_LIGHT );
					}
				}
				break;

				default: break;
			};
		} // for z
	} // for xy neighbors
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

void h_World::UpdatePhysMesh( short x_min, short x_max, short y_min, short y_max, short z_min, short z_max )
{
	p_WorldPhysMesh phys_mesh;

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
				phys_mesh.upper_block_faces.emplace_back( x + X, y + Y, z + 1, t > t_up ? h_Direction::Down : h_Direction::Up );
			}
			if( t != t_fr )
			{
				if( t > t_fr )
					phys_mesh.block_sides.emplace_back( x + X + 1, y + Y + ((x+1)&1), z, h_Direction::BackLeft );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z, h_Direction::ForwardRight );
			}
			if( t != t_br )
			{
				if( t > t_br )
					phys_mesh.block_sides.emplace_back( x + X + 1, y + Y - (x&1), z, h_Direction::ForwardLeft );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z, h_Direction::BackRight );
			}
			if( t!= t_f )
			{
				if( t > t_f )
					phys_mesh.block_sides.emplace_back( x + X, y + Y + 1, z, h_Direction::Back );
				else
					phys_mesh.block_sides.emplace_back( x + X, y + Y, z, h_Direction::Forward );
			}
		} // for z
	} // for xy

	for( short x= x_min; x< x_max; x++ )
	for( short y= y_min; y< y_max; y++ )
	{
		h_Chunk* chunk= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
		const h_Block* const* blocks=
			chunk->GetBlocksData() +
			BlockAddr( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), 0 );

		for( short z= z_min; z < z_max; z++ )
		{
			if( blocks[z]->Type() == h_BlockType::Water )
			{
				const h_LiquidBlock* water_block= static_cast<const h_LiquidBlock*>( blocks[z] );

				p_WaterBlock water_phys_block;
				water_phys_block.x= x + X;
				water_phys_block.y= y + Y;
				water_phys_block.z= z;
				water_phys_block.water_level= float(water_block->LiquidLevel()) / float(H_MAX_WATER_LEVEL);

				phys_mesh.water_blocks.push_back( water_phys_block );
			}
		}
	}

	std::lock_guard<std::mutex> lock( phys_mesh_mutex_ );
	phys_mesh_= std::make_shared< p_WorldPhysMesh >( std::move(phys_mesh ) );
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
		while(phys_thread_paused_.load())
			hSleep( g_sleep_interval_on_pause );

		H_ASSERT( player_ );
		H_ASSERT( renderer_ );

		TestMobTick();

		uint64_t t0_ms = hGetTimeMS();

		FlushActionQueue();
		WaterPhysTick();
		{
			for( unsigned int y= active_area_margins_[1]; y < chunk_number_y_ - active_area_margins_[1]; y++ )
			for( unsigned int x= active_area_margins_[0]; x < chunk_number_x_ - active_area_margins_[0]; x++ )
				GetChunk( x, y )->ProcessFailingBlocks();
		}
		GrassPhysTick();
		RelightWaterModifedChunksLight();

		// player logic
		{
			m_Vec3 player_pos= player_->EyesPos();
			short player_coord_global[2];
			pGetHexogonCoord( player_pos.xy(), &player_coord_global[0], &player_coord_global[1] );

			int player_coord[3];
			player_coord[0]= player_coord_global[0] - Longitude() * H_CHUNK_WIDTH;
			player_coord[1]= player_coord_global[1] - Latitude () * H_CHUNK_WIDTH;
			player_coord[2]= int( std::round(player_pos.z) );
			UpdatePhysMesh(
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
		}

		phys_tick_count_++;

		renderer_->Update();

		uint64_t t1_ms= hGetTimeMS();
		unsigned int dt_ms= (unsigned int)(t1_ms - t0_ms);
		if (dt_ms < g_update_inrerval_ms)
			hSleep( g_update_inrerval_ms - dt_ms );
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

			m_Collection< h_LiquidBlock* >& l= ch->water_block_list_;
			h_LiquidBlock* b;
			m_Collection< h_LiquidBlock* >::Iterator iter(&l);
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

void h_World::GrassPhysTick()
{
	const unsigned int c_reproducing_start_chance= m_Rand::max_rand / 16;
	const unsigned int c_reproducing_do_chance= m_Rand::max_rand / 6;
	const unsigned char c_min_light_for_grass_reproducing= H_MAX_SUN_LIGHT / 2;

	m_Vec3 sun_vector= calendar_.GetSunVector( phys_tick_count_, GetGlobalWorldLatitude() );
	unsigned char current_sun_multiplier= sun_vector.z > std::sin( 4.0f * m_Math::deg2rad ) ? 1 : 0;

	for( unsigned int y= active_area_margins_[1]; y < chunk_number_y_ - active_area_margins_[1]; y++ )
	for( unsigned int x= active_area_margins_[0]; x < chunk_number_x_ - active_area_margins_[0]; x++ )
	{
		h_Chunk* chunk= GetChunk( x, y );
		int X= x << H_CHUNK_WIDTH_LOG2;
		int Y= y << H_CHUNK_WIDTH_LOG2;

		auto& blocks= chunk->active_grass_blocks_;
		for( unsigned int i= 0; i < blocks.size(); )
		{
			h_GrassBlock* grass_block= blocks[i];

			H_ASSERT( grass_block->IsActive() );
			H_ASSERT( grass_block->GetZ() > 0 );

			int block_addr= BlockAddr( grass_block->GetX(), grass_block->GetY(), grass_block->GetZ() );
			H_ASSERT( block_addr <= H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

			H_ASSERT( chunk->blocks_[ block_addr ] == grass_block );

			// Grass can exist only if upper block is air.
			if( chunk->blocks_[ block_addr + 1 ]->Type() != h_BlockType::Air )
			{
				chunk->blocks_[ block_addr ]= NormalBlock( h_BlockType::Soil );

				chunk->active_grass_blocks_allocator_.Delete( grass_block );

				if( i != blocks.size() - 1 ) blocks[i]= blocks.back();
				blocks.pop_back();

				renderer_->UpdateChunk( x, y );

				continue;
			}

			unsigned char light=
				chunk-> sun_light_map_[ block_addr + 1 ] * current_sun_multiplier +
				chunk->fire_light_map_[ block_addr + 1 ];

			if( light >= c_min_light_for_grass_reproducing &&
				phys_processes_rand_.Rand() <= c_reproducing_start_chance )
			{
				bool can_reproduce= false;

				bool z_plus_2_block_is_air= chunk->blocks_[ block_addr + 2 ]->Type() == h_BlockType::Air;

				int world_x= grass_block->GetX() + X;
				int world_y= grass_block->GetY() + Y;

				int forward_side_y= world_y + ( (world_x^1) & 1 );
				int back_side_y= world_y - (world_x & 1);

				int neighbors[6][2]=
				{
					{ world_x, world_y + 1 },
					{ world_x, world_y - 1 },
					{ world_x + 1, forward_side_y },
					{ world_x + 1, back_side_y },
					{ world_x - 1, forward_side_y },
					{ world_x - 1, back_side_y },
				};

				for( unsigned int n= 0; n < 6; n++ )
				{
					int neinghbor_chunk_x= neighbors[n][0] >> H_CHUNK_WIDTH_LOG2;
					int neinghbor_chunk_y= neighbors[n][1] >> H_CHUNK_WIDTH_LOG2;

					h_Chunk* neighbor_chunk= GetChunk( neinghbor_chunk_x, neinghbor_chunk_y );

					int local_x= neighbors[n][0] & (H_CHUNK_WIDTH-1);
					int local_y= neighbors[n][1] & (H_CHUNK_WIDTH-1);
					int neighbor_addr= BlockAddr( local_x, local_y, grass_block->GetZ() );
					H_ASSERT( neighbor_addr <= H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

					h_BlockType z_minus_one_block_type= neighbor_chunk->blocks_[ neighbor_addr - 1 ]->Type();
					h_BlockType neighbor_block_type   = neighbor_chunk->blocks_[ neighbor_addr     ]->Type();
					h_BlockType z_plus_one_block_type = neighbor_chunk->blocks_[ neighbor_addr + 1 ]->Type();
					h_BlockType z_plus_two_block_type = neighbor_chunk->blocks_[ neighbor_addr + 2 ]->Type();

					if( z_minus_one_block_type == h_BlockType::Soil &&
						neighbor_block_type    == h_BlockType::Air &&
						z_plus_one_block_type  == h_BlockType::Air )
					{
						if( phys_processes_rand_.Rand() <= c_reproducing_do_chance )
						{
							neighbor_chunk->blocks_[ neighbor_addr - 1 ]=
								neighbor_chunk->NewActiveGrassBlock(
									local_x, local_y, grass_block->GetZ() - 1 );

							renderer_->UpdateChunk( neinghbor_chunk_x, neinghbor_chunk_y );
						}
						can_reproduce= true;
					}
					if( neighbor_block_type    == h_BlockType::Soil &&
						z_plus_one_block_type  == h_BlockType::Air )
					{
						if( phys_processes_rand_.Rand() <= c_reproducing_do_chance )
						{
							neighbor_chunk->blocks_[ neighbor_addr  ]=
								neighbor_chunk->NewActiveGrassBlock(
									local_x, local_y, grass_block->GetZ() );

							renderer_->UpdateChunk( neinghbor_chunk_x, neinghbor_chunk_y );
						}
						can_reproduce= true;
					}
					if( z_plus_one_block_type  == h_BlockType::Soil &&
						z_plus_two_block_type  == h_BlockType::Air &&
						z_plus_2_block_is_air )
					{
						if( phys_processes_rand_.Rand() <= c_reproducing_do_chance )
						{
							neighbor_chunk->blocks_[ neighbor_addr + 1 ]=
								neighbor_chunk->NewActiveGrassBlock(
									local_x, local_y, grass_block->GetZ() + 1 );

							renderer_->UpdateChunk( neinghbor_chunk_x, neinghbor_chunk_y );
						}
						can_reproduce= true;
					}

				} // for neighbors

				if( !can_reproduce )
				{
					// Deactivate grass block
					chunk->blocks_[ block_addr ]= &unactive_grass_block_;

					chunk->active_grass_blocks_allocator_.Delete( grass_block );

					if( i != blocks.size() - 1 ) blocks[i]= blocks.back();
					blocks.pop_back();

					continue;
				}

			} // if rand

			i++;
		} // for grass blocks
	} // for chunks
}

void h_World::InitNormalBlocks()
{
	for( size_t i= 0; i < size_t(h_BlockType::NumBlockTypes); i++ )
		new ( normal_blocks_ + i ) h_Block( h_BlockType( static_cast<h_BlockType>(i) ) );
}
