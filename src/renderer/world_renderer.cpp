#include <vector>

#include "world_renderer.hpp"
#include "world_vertex_buffer.hpp"
#include "glcorearb.h"
#include "rendering_constants.hpp"
#include "../console.hpp"
#include "ogl_state_manager.hpp"
#include "img_utils.hpp"
#include "../math_lib/m_math.h"

#include "../world.hpp"

r_WorldRenderer::r_WorldRenderer( const h_World* world )
	: world_(world)
	, host_data_mutex_(), gpu_data_mutex_()
	, settings_( "config.ini", QSettings::IniFormat )
	, frame_count(0), update_count(0)
	, sun_vector_( 0.7f, 0.8f, 0.6f )
{
	world_vb_.need_update_vbo= false;

	startup_time_= QTime::currentTime();

	last_fps= 0;
	last_fps_time= QTime::currentTime();
	frames_in_last_second= 0;
	chunk_updates_in_last_second = chunk_updates_per_second= 0;
	chunks_rebuild_per_second= chunk_rebuild_in_last_second= 0;

	water_quadchunks_updates_per_second= water_quadchunks_updates_in_last_second= 0;
	water_quadchunks_rebuild_per_second= water_quadchunks_rebuild_in_last_second= 0;

	updade_ticks_per_second= update_ticks_in_last_second= 0;
}

r_WorldRenderer::~r_WorldRenderer()
{
	//update_thread.killTimer(
	//update_thread.wait();
	//update_thread.exit();
}

void r_WorldRenderer::UpdateChunk(unsigned short X,  unsigned short Y )
{
	if( frame_count == 0 )
		return;
	else
		chunk_info_[ X + Y * world_->ChunkNumberX() ].chunk_data_updated_= true;
}

void r_WorldRenderer::UpdateChunkWater(unsigned short X,  unsigned short Y )
{
	if( frame_count == 0 )
		return;
	else
		chunk_info_[ X + Y * world_->ChunkNumberX() ].chunk_water_data_updated_= true;
}

void r_WorldRenderer::FullUpdate()
{
	return;

	if( frame_count == 0 )
		return;
	for( unsigned int i= 0; i< world_->ChunkNumberX(); i++ )
		for( unsigned int j= 0; j< world_->ChunkNumberY(); j++ )
		{
			r_ChunkInfo* ch= &chunk_info_[ i + j * world_->ChunkNumberX() ];
			ch->chunk_data_updated_= true;
			ch->chunk_water_data_updated_= true;
			ch->chunk_= world_->GetChunk( i, j );
			if( j!= 0 )
			{
				ch->chunk_back_= world_->GetChunk( i, j-1 );
				if( i!= world_->ChunkNumberX() - 1 )
					ch->chunk_back_right_= world_->GetChunk( i + 1, j - 1 );
				else
					ch->chunk_back_right_= nullptr;
			}
			else
				ch->chunk_back_right_= ch->chunk_back_= nullptr;

			if( j!= world_->ChunkNumberY() - 1 )
				ch->chunk_front_= world_->GetChunk( i, j+1 );
			else
				ch->chunk_front_= nullptr;

			if( i!= world_->ChunkNumberX() - 1 )
				ch->chunk_right_= world_->GetChunk( i+1, j );
			else
				ch->chunk_right_= nullptr;

			/*if( i < world_->ChunkNumberX() - 1 )
			    chunk_info_[k].chunk_right= world_->GetChunk( i + 1, j );
			if( j< world_->ChunkNumberY() - 1 )
			    chunk_info_[k].chunk_front= world_->GetChunk( i, j + 1 );
			if( j > 0 )
			    chunk_info_[k].chunk_back= world_->GetChunk( i, j - 1 );
			if( j > 0 && i < world_->ChunkNumberX() - 1 )
			    chunk_info_[k].chunk_back_right= world_->GetChunk( i + 1, j - 1 );*/
		}
}

void r_WorldRenderer::UpdateWorld()
{	return;
	host_data_mutex_.lock();


	/*for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		for( int i= 0; i< world_vertex_buffer_.chunks_per_cluster_x_; i++ )
		{
			int k=  i + j * r_WorldVBO::MAX_CLUSTERS_;
			r_WorldVBO::r_WorldVBOCluster* cluster= world_vertex_buffer_.clusters_[k];

			for( int y= 0; y< world_vertex_buffer_.cluster_matrix_size_y_; y++ )
				for( int x= 0; x< world_vertex_buffer_.cluster_matrix_size_x_; x++ )
				{
					r_WorldVBO::r_WorldVBOCluster::r_ChunkVBOData* vbo_data= cluster->GetChunkVBOData( x, y );
					if( vbo_data->updated_ )
					{

					}
				}// for chunks in cluster
		}// for chunks*/

	// calculate chunk vertex count, mark clusters, which need reallocate
	for( int y= 0; y< chunk_num_y_; y++ )
		for( int x= 0; x< chunk_num_x_; x++ )
		{
			int n= x + y * chunk_num_x_;
			if( chunk_info_[n].chunk_data_updated_ )
			{
				chunk_info_[n].GetQuadCount();
				r_WorldVBO::r_WorldVBOCluster* cluster;
				auto chunk_data= world_vertex_buffer_.
								 GetChunkDataForGlobalCoordinates( chunk_info_[n].chunk_->Longitude(), chunk_info_[n].chunk_->Latitude() );
				chunk_data->updated_= true;
				chunk_data->vertex_count_= chunk_info_[n].chunk_vb_.new_vertex_count;
				if( chunk_data->vertex_count_ > chunk_data->reserved_vertex_count_ )
				{
					world_vertex_buffer_.
					GetClusterForGlobalCoordinates( chunk_info_[n].chunk_->Longitude(), chunk_info_[n].chunk_->Latitude() )->
					need_reallocate_vbo_= true;
				}
			}
		}

	// reallocate memory in clusters, where it need
	for( int j= 0; j< world_vertex_buffer_.chunks_per_cluster_y_; j++ )
		for( int i= 0; i< world_vertex_buffer_.chunks_per_cluster_x_; i++ )
		{
			r_WorldVBO::r_WorldVBOCluster* cluster= world_vertex_buffer_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ];
			if( cluster-> need_reallocate_vbo_ )
			{
				r_WorldVBO::r_WorldVBOCluster* new_cluster=
					new r_WorldVBO::r_WorldVBOCluster( cluster->longitude_, cluster->latitude_, &world_vertex_buffer_ );
				*new_cluster= *cluster;
				// now, we have copy of cluster. copy to new cluster vertex data inside old cluster, for chunks, which do not need rebuild

				new_cluster->PrepareBufferSizes();
				new_cluster->vbo_data_= new r_WorldVertex[ new_cluster->vbo_vertex_count_ ];

				int own_chunk_matrix_x= cluster->longitude_ - chunk_info_[0].chunk_->Longitude();
				int own_chunk_matrix_y= cluster->latitude_  - chunk_info_[0].chunk_->Latitude() ;
				for( int y= 0; y< world_vertex_buffer_.chunks_per_cluster_y_; y++ )
					for( int x= 0; x< world_vertex_buffer_.chunks_per_cluster_x_; x++ )
					{
						r_WorldVBO::r_WorldVBOCluster::r_ChunkVBOData* chunk_data= new_cluster->GetChunkVBOData( x, y );
						if( !chunk_data->updated_ )
						{
							r_WorldVBO::r_WorldVBOCluster::r_ChunkVBOData* old_chunk_data= cluster->GetChunkVBOData( x, y );
							memcpy( cluster->vbo_data_ + old_chunk_data->vbo_offset_,
									new_cluster->vbo_data_ + chunk_data->vbo_offset_, chunk_data->vertex_count_ * sizeof(r_WorldVertex) );
						}
						// set chunk pointer to data
						chunk_info_[ own_chunk_matrix_x + x + (own_chunk_matrix_y+y) * chunk_num_x_ ].chunk_vb_.vb_data=
							new_cluster->vbo_data_ + chunk_data->vbo_offset_;
					}//for chunks in cluster

				delete[] cluster->vbo_data_;
				delete cluster;
				world_vertex_buffer_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ]= new_cluster;
			}// if need rebuild all cluster
		}

	for( int y= 0; y< chunk_num_y_; y++ )
		for( int x= 0; x< chunk_num_x_; x++ )
		{
			int n= x + y * chunk_num_x_;
			if( chunk_info_[n].chunk_data_updated_ )
			{
				//auto chunk_data= world_vertex_buffer_.
				//	GetChunkDataForGlobalCoordinates( chunk_info_[n].chunk->Longitude(), chunk_info_[n].chunk->Latitude() );
				chunk_info_[n].BuildChunkMesh();
				chunk_info_[n].chunk_data_updated_= false;

				auto chunk_data= world_vertex_buffer_.
								 GetChunkDataForGlobalCoordinates( chunk_info_[n].chunk_->Longitude(), chunk_info_[n].chunk_->Latitude() );
				chunk_data->updated_= false;
				chunk_data->rebuilded_= true;
			}
		}

	host_data_mutex_.unlock();

	/*host_data_mutex.lock();
	world_->Lock();

	unsigned int i, j, n;
	bool full_update= false;
	bool any_vbo_updated= false;
	r_WorldVertex *old_chunk_vb_data;
	unsigned int vb_shift;
	unsigned int new_allocated_vertex_count= 0;

	for( i= 0; i< world_->ChunkNumberX(); i++ )
	    for( j= 0; j< world_->ChunkNumberY(); j++ )
	    {
	        n=  i + j * world_->ChunkNumberX();
	        if( chunk_info_[n].chunk_data_updated )
	        {
	            chunk_info_[n].GetQuadCount();
	            if( chunk_info_[n].chunk_vb.new_vertex_count > chunk_info_[n].chunk_vb.allocated_vertex_count )
	                full_update= true;
	        }
	    }

	if( full_update )
	{
	    h_Console::Message( "full update" );
	    for( i= 0; i< world_->ChunkNumberX(); i++ )
	        for( j= 0; j< world_->ChunkNumberY(); j++ )
	        {
	            n=  i + j * world_->ChunkNumberX();
	            if( chunk_info_[n].chunk_data_updated )
	                new_allocated_vertex_count+=
	                    chunk_info_[n].chunk_vb.allocated_vertex_count= chunk_info_[n].chunk_vb.new_vertex_count +
	                            ((chunk_info_[n].chunk_vb.new_vertex_count >> 2 )& 0xFFFFFFFC );
	            else
	                new_allocated_vertex_count+=
	                    chunk_info_[n].chunk_vb.allocated_vertex_count= chunk_info_[n].chunk_vb.real_vertex_count +
	                            ((chunk_info_[n].chunk_vb.real_vertex_count >> 2)& 0xFFFFFFFC );
	        }

	    world_vb.new_vb_data= new r_WorldVertex[ new_allocated_vertex_count ];

	    vb_shift= 0;
	    for( i= 0; i< world_->ChunkNumberX(); i++ )
	        for( j= 0; j< world_->ChunkNumberY(); j++ )
	        {
	            n=  i + j * world_->ChunkNumberX();
	            old_chunk_vb_data= chunk_info_[n].chunk_vb.vb_data;
	            chunk_info_[n].chunk_vb.vb_data= world_vb.new_vb_data + vb_shift;


	            if( !chunk_info_[n].chunk_data_updated )
	            {
	                //copy old chunk mesh data to new data array
	                memcpy( chunk_info_[n].chunk_vb.vb_data, old_chunk_vb_data,
	                        sizeof(r_WorldVertex) * chunk_info_[n].chunk_vb.real_vertex_count );
	            }
	            vb_shift+= chunk_info_[n].chunk_vb.allocated_vertex_count;

	        }
	    // world_vb.need_update_vbo= true;
	}//if full update


	for( i= 0; i< world_->ChunkNumberX(); i++ )
	    for( j= 0; j< world_->ChunkNumberY(); j++ )
	    {
	        n=  i + j * world_->ChunkNumberX();
	        if( chunk_info_[n].chunk_data_updated )
	        {
	            chunk_info_[n].BuildChunkMesh();
	            chunk_info_[n].chunk_data_updated= false;
	            chunk_info_[n].chunk_mesh_rebuilded= true;
	            any_vbo_updated= true;
	            chunk_rebuild_in_last_second++;
	        }
	    }

	world_->Unlock();
	host_data_mutex.unlock();

	if( any_vbo_updated )
	{
	    gpu_data_mutex.lock();
	    if( full_update )
	    {
	        delete[] world_vb.vb_data;
	        world_vb.allocated_vertex_count= new_allocated_vertex_count;
	        world_vb.need_update_vbo= true;
	    }

	    world_vb.vbo_update_ready= true;
	    gpu_data_mutex.unlock();
	}*/
}

void r_WorldRenderer::UpdateWater()
{
	host_data_mutex_.lock();

	unsigned int i, j;
	bool any_vbo_updated= false;
	bool full_update= false;

	// r_WaterVertex* new_vb_data;
	unsigned int new_vertex_count;

	//build water meshes for CHUNKS
	for( i= 0; i< chunk_num_x_; i++ )
		for( j= 0 ; j< chunk_num_y_; j++ )
		{
			r_ChunkInfo* ch= &chunk_info_[ i + j * chunk_num_x_ ];
			if( ch->chunk_water_data_updated_ )
				ch->BuildWaterSurfaceMesh();
		}

	//calculate new vertex count for QUADchunks
	for( i= 0; i< quadchunk_num_x_; i++ )
		for( j= 0; j< quadchunk_num_y_; j++ )
		{
			r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
			ch->GetVertexCount();
			ch->GetUpdatedState();
			if( ch->new_vertex_count_ > ch->allocated_vertex_count_ )
				full_update= true;
		}

	//reset update state of water data for CHUNKS
	for( i= 0; i< chunk_num_x_; i++ )
		for( j= 0 ; j< chunk_num_y_; j++ )
		{
			r_ChunkInfo* ch= &chunk_info_[ i + j * chunk_num_x_ ];
			ch->chunk_water_data_updated_= false;
		}

	if( full_update )
	{
		//calculate size of new VBO
		new_vertex_count= 0;
		for( i= 0; i< quadchunk_num_x_; i++ )
			for( j= 0; j< quadchunk_num_y_; j++ )
			{
				r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
				if( ch->water_updated_ )
					ch->allocated_vertex_count_= ch->new_vertex_count_ + ( ch->new_vertex_count_ >> 2 );// +25%
				else
					ch->allocated_vertex_count_= ch->real_vertex_count_ + ( ch->real_vertex_count_ >> 2 );// +25%
				new_vertex_count+= ch->allocated_vertex_count_;
			}

		//copy old unmodifed quadchunk water data to new VBO and change r_WaterQuadChunkInfo::vb_data to new
		water_vb_.new_vb_data= new r_WaterVertex[ new_vertex_count ];
		r_WaterVertex* new_quadchunk_vb_data= water_vb_.new_vb_data;
		for( i= 0; i< quadchunk_num_x_; i++ )
			for( j= 0; j< quadchunk_num_y_; j++ )
			{
				r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
				r_WaterVertex* old_vb_data= ch->vb_data_;
				ch->vb_data_= new_quadchunk_vb_data;


				if( ! ch->water_updated_ )
				{
					memcpy( ch->vb_data_, old_vb_data,
							sizeof(r_WaterVertex ) * ch->real_vertex_count_ );
				}
				new_quadchunk_vb_data+= ch->allocated_vertex_count_;
			}
	}// if( full_update )

	for( i= 0; i< quadchunk_num_x_; i++ )
		for( j= 0; j< quadchunk_num_y_; j++ )
		{
			r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
			if( ch->water_updated_ )
			{
				ch->BuildFinalMesh();
				ch->water_updated_= false;
				ch->water_mesh_rebuilded_= true;
				any_vbo_updated= true;
				water_quadchunks_rebuild_in_last_second++;
			}
		}

	host_data_mutex_.unlock();

	if( any_vbo_updated )
	{
		gpu_data_mutex_.lock();
		water_vb_.vbo_update_ready= true;
		if( full_update )
		{
			delete[] water_vb_.vb_data;
			//water_vb_.vb_data= water_vb_.new_vb_data;
			water_vb_.need_update_vbo= true;
			water_vb_.allocated_vertex_count= new_vertex_count;
		}
		gpu_data_mutex_.unlock();
	}
}

void r_WorldRenderer::UpdateFunc()
{
	return;

	//while(1)
	//{
	QTime t0= QTime::currentTime();

	UpdateWorld();
	if( update_count&1 )
		UpdateWater();

	QTime t1= QTime::currentTime();
	unsigned int dt_ms= t0.msecsTo( t1 );
	if( dt_ms < 50 )
		usleep( (50 - dt_ms) * 1000);

	update_count++;
	update_ticks_in_last_second++;

	//}
}

void r_WorldRenderer::UpdateGPUData()
{	return;
	/*gpu_data_mutex.lock();
	if( world_vb.vbo_update_ready )
	{
	    host_data_mutex.lock();

	    unsigned int i, j, n;

	    if( world_vb.need_update_vbo )
	    {
	        world_vb.vb_data= world_vb.new_vb_data;
	        world_vb.vbo.VertexData( (float*) world_vb.vb_data, world_vb.allocated_vertex_count * sizeof( r_WorldVertex ),
	                                 sizeof( r_WorldVertex ) );
	        for( i= 0; i< world_->ChunkNumberX(); i++ )
	            for( j= 0; j< world_->ChunkNumberY(); j++ )
	            {
	                n=  i + j * world_->ChunkNumberX();
	                if( chunk_info_[n].chunk_mesh_rebuilded )
	                {
	                    chunk_info_[n].chunk_mesh_rebuilded= false;
	                    chunk_info_[n].chunk_vb.real_vertex_count= chunk_info_[n].chunk_vb.new_vertex_count;
	                }
	            }

	        memcpy( chunk_info_to_draw, chunk_info,
	                sizeof( r_ChunkInfo ) * world_->ChunkNumberX() * world_->ChunkNumberY() );

	        world_vb.need_update_vbo= false;
	        chunk_updates_in_last_second+= world_->ChunkNumberX() * world_->ChunkNumberY();
	        h_Console::Message( "GPU full update" );
	    }
	    else
	        for( i= 0; i< world_->ChunkNumberX(); i++ )
	            for( j= 0; j< world_->ChunkNumberY(); j++ )
	            {
	                n=  i + j * world_->ChunkNumberX();
	                if( chunk_info_[n].chunk_mesh_rebuilded )
	                {
	                    world_vb.vbo.VertexSubData( chunk_info_[n].chunk_vb.vb_data,
	                                                chunk_info_[n].chunk_vb.new_vertex_count * sizeof( r_WorldVertex ),
	                                                (unsigned int)( chunk_info_[n].chunk_vb.vb_data - world_vb.vb_data ) * sizeof( r_WorldVertex ) );

	                    chunk_info_[n].chunk_mesh_rebuilded= false;
	                    chunk_info_[n].chunk_vb.real_vertex_count= chunk_info_[n].chunk_vb.new_vertex_count;

	                    memcpy( &chunk_info_to_draw[n], &chunk_info_[n], sizeof( r_ChunkInfo ) );
	                    chunk_updates_in_last_second++;
	                }
	            }

	    host_data_mutex.unlock();

	    world_vb.vbo_update_ready = false;
	}//if( world_vb.vbo_update_ready )
	gpu_data_mutex.unlock();
	*/

	if( host_data_mutex_.try_lock() )
	{
		for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		{
			for( int i= 0; i< world_vertex_buffer_.cluster_matrix_size_x_; i++ )
			{
				int k=  i + j * r_WorldVBO::MAX_CLUSTERS_;
			}
		}
		host_data_mutex_.unlock();
	}
//water update

	gpu_data_mutex_.lock();
	if( water_vb_.vbo_update_ready )
	{
		host_data_mutex_.lock();

		if( water_vb_.need_update_vbo )
		{
			water_vb_.vb_data= water_vb_.new_vb_data;
			water_vb_.vbo.VertexData( water_vb_.vb_data,
									 water_vb_.allocated_vertex_count * sizeof( r_WaterVertex ),
									 sizeof( r_WaterVertex ) );
			for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
				for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
				{
					r_WaterQuadChunkInfo* ch= & water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
					if( ch->water_mesh_rebuilded_ )
					{
						ch->water_mesh_rebuilded_= false;
						ch->real_vertex_count_= ch->new_vertex_count_;
					}
				}

			water_vb_.need_update_vbo= false;

			memcpy( water_quadchunk_info_to_draw_, water_quadchunk_info_,
					sizeof( r_WaterQuadChunkInfo ) * quadchunk_num_x_ * quadchunk_num_y_ );

			water_quadchunks_updates_in_last_second+= quadchunk_num_x_ * quadchunk_num_y_;
			h_Console::Message( "GPU water full update" );
		}//if( water_vb_.need_update_vbo )
		else
		{
			for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
				for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
				{
					r_WaterQuadChunkInfo* ch= & water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
					if( ch->water_mesh_rebuilded_ )
					{
						water_vb_.vbo.VertexSubData(
							ch->vb_data_,
							ch->new_vertex_count_ * sizeof( r_WaterVertex ),
							(ch->vb_data_ - water_vb_.vb_data) * sizeof( r_WaterVertex ) );

						ch->water_mesh_rebuilded_= false;
						ch->real_vertex_count_= ch->new_vertex_count_;
						memcpy( & water_quadchunk_info_to_draw_[ i + j * quadchunk_num_x_ ],
								& water_quadchunk_info_[ i + j * quadchunk_num_x_ ],
								sizeof( r_WaterQuadChunkInfo )  );

						water_quadchunks_updates_in_last_second++;
					}
				}
		}

		water_vb_.vbo_update_ready= false;
		host_data_mutex_.unlock();
	}
	gpu_data_mutex_.unlock();
}

void r_WorldRenderer::CalculateMatrices()
{
	m_Mat4 scale, translate, result, perspective, rotate_x, rotate_z, basis_change;

	translate.Translate( -cam_pos_ );

	static const m_Vec3 s_vector(
		H_BLOCK_SCALE_VECTOR_X,
		H_BLOCK_SCALE_VECTOR_Y,
		H_BLOCK_SCALE_VECTOR_Z );//hexogonal prism scale vector. DO NOT TOUCH!
	scale.Scale( s_vector );

	perspective.PerspectiveProjection(
		float(viewport_width_)/float(viewport_height_), 1.57f,
		0.5f*(H_PLAYER_HEIGHT - H_PLAYER_EYE_LEVEL),//znear
		1024.0f );

	rotate_x.RotateX( -cam_ang_.x );
	rotate_z.RotateZ( -cam_ang_.z );

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;
	view_matrix_= translate * rotate_z * rotate_x * basis_change * perspective;

	block_scale_matrix_= scale;
	block_final_matrix_= block_scale_matrix_ * view_matrix_;
}

void r_WorldRenderer::CalculateLight()
{
	lighting_data_.current_sun_light= R_SUN_LIGHT_COLOR / float ( H_MAX_SUN_LIGHT * 16 );
	lighting_data_.current_fire_light= R_FIRE_LIGHT_COLOR / float ( H_MAX_FIRE_LIGHT * 16 );
}

void r_WorldRenderer::BuildChunkList()
{
	unsigned int k;
	r_ChunkInfo* ch;
	for( unsigned int i=0; i< world_->ChunkNumberX(); i++ )
		for( unsigned int j= 0; j< world_->ChunkNumberY(); j++ )
		{
			k= i + j * world_->ChunkNumberX();
			ch= &chunk_info_to_draw_[ k ];
			world_vb_.chunk_meshes_index_count[k]= ch->chunk_vb_.real_vertex_count * 6 / 4;
			world_vb_.base_vertices[k]= ch->chunk_vb_.vb_data - world_vb_.vb_data;
			world_vb_.multi_indeces[k]= nullptr;
		}

	world_vb_.chunks_to_draw= world_->ChunkNumberX() * world_->ChunkNumberY();

	k= 0;
	for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
		for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
		{
			r_WaterQuadChunkInfo* qch= &water_quadchunk_info_to_draw_[ i + j * quadchunk_num_x_ ];
			if( qch->real_vertex_count_ != 0 )
			{
				water_vb_.chunk_meshes_index_count[k]= qch->real_vertex_count_ * 2;//hexogon has 4 triangle => 12 indeces per 6 vertices
				water_vb_.base_vertices[k]= qch->vb_data_ - water_vb_.vb_data;
				water_vb_.multi_indeces[k]= nullptr;
				k++;
			}
		}
	water_vb_.quadchunks_to_draw= k;
}

void r_WorldRenderer::Draw()
{
	if( frame_count == 0 )
	{

	}

	UpdateGPUData();
	CalculateMatrices();
	CalculateLight();

	//supersampling_buffer.Bind();
	//glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	BuildChunkList();
	DrawWorld();
	DrawSky();
	DrawSun();
	weather_effects_particle_manager_.Draw( view_matrix_, cam_pos_ );

	DrawBuildPrism();
	DrawWater();

	/*r_Framebuffer::BindScreenFramebuffer();
	r_OGLStateManager::DisableBlend();
	r_OGLStateManager::DisableDepthTest();
	r_OGLStateManager::DepthMask(false);
	supersampling_final_shader.Bind();
	(*supersampling_buffer.GetTextures())[0].Bind(0);
	supersampling_final_shader.Uniform( "frame_buffer", 0 );
	glDrawArrays( GL_TRIANGLES, 0, 6 );*/

	DrawConsole();

	if( settings_.value( "show_debug_info", false ).toBool() && h_Console::GetPosition() == 0.0f )
	{
		float text_scale= 0.25f;
		text_manager_->AddMultiText( 0, 0, text_scale, r_Text::default_color, "fps: %d", last_fps );
		text_manager_->AddMultiText( 0, 1, text_scale, r_Text::default_color, "chunks: %dx%d", chunk_num_x_, chunk_num_y_ );
		text_manager_->AddMultiText( 0, 2, text_scale, r_Text::default_color, "chunks updated per second: %d", chunk_updates_per_second );
		text_manager_->AddMultiText( 0, 3, text_scale, r_Text::default_color, "water quadchunks updated per second: %d", water_quadchunks_updates_per_second );
		text_manager_->AddMultiText( 0, 4, text_scale, r_Text::default_color, "update ticks per second: %d",updade_ticks_per_second );
		text_manager_->AddMultiText( 0, 5, text_scale, r_Text::default_color, "cam pos: %4.1f %4.1f %4.1f",
									cam_pos_.x, cam_pos_.y, cam_pos_.z );

		//text_manager->AddMultiText( 0, 11, text_scale, r_Text::default_color, "quick brown fox jumps over the lazy dog\nQUICK BROWN FOX JUMPS OVER THE LAZY DOG\n9876543210-+/\\" );
		//text_manager->AddMultiText( 0, 0, 8.0f, r_Text::default_color, "#A@Kli\nO01-eN" );

		//text_manager->AddMultiText( 0, 1, 4.0f, r_Text::default_color, "QUICK BROWN\nFOX JUMPS OVER\nTHE LAZY DOG" );

		text_manager_->Draw();
	}
	CalculateFPS();
}

void r_WorldRenderer::DrawConsole()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );

	h_Console::Move( 0.016f );

	if( h_Console::GetPosition() == 0.0f )
		return;

	r_OGLStateManager::UpdateState( state );

	console_bg_shader_.Bind();
	console_bg_shader_.Uniform( "tex", 0 );
	console_bg_shader_.Uniform( "pos", 1.0f - h_Console::GetPosition() );
	m_Vec3 scr_size( viewport_width_, viewport_height_, 0.0f );
	console_bg_shader_.Uniform( "screen_size", scr_size );

	console_bg_texture_.Bind(0);

	glDrawArrays( GL_POINTS, 0, 1 );
	h_Console::Draw( text_manager_ );
	text_manager_->Draw();
}

void r_WorldRenderer::CalculateFPS()
{
	frames_in_last_second++;

	if( last_fps_time.msecsTo( QTime::currentTime() ) > 1000 )
	{
		last_fps= frames_in_last_second;
		frames_in_last_second= 0;
		last_fps_time= QTime::currentTime();

		chunk_updates_per_second= chunk_updates_in_last_second;
		chunk_updates_in_last_second= 0;

		chunks_rebuild_per_second= chunk_rebuild_in_last_second;
		chunk_rebuild_in_last_second= 0;


		water_quadchunks_rebuild_per_second= water_quadchunks_rebuild_in_last_second;
		water_quadchunks_rebuild_in_last_second= 0;

		water_quadchunks_updates_per_second= water_quadchunks_updates_in_last_second;
		water_quadchunks_updates_in_last_second= 0;

		updade_ticks_per_second= update_ticks_in_last_second;
		update_ticks_in_last_second= 0;
	}
	frame_count++;
}

void r_WorldRenderer::DrawWorld()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		false, true, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );
	r_OGLStateManager::UpdateState( state );

	texture_manager_.BindTextureArray( 0 );

	world_shader_.Bind();
	world_shader_.Uniform( "tex", 0 );
	world_shader_.Uniform( "sun_vector", sun_vector_ );
	world_shader_.Uniform( "view_matrix", block_final_matrix_ );

	world_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	world_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	world_shader_.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

	/*world_vb.vbo.Bind();
	glMultiDrawElementsBaseVertex( GL_TRIANGLES, world_vb.chunk_meshes_index_count, GL_UNSIGNED_SHORT,
	                               (const GLvoid**)(world_vb.multi_indeces), world_vb.chunks_to_draw,
	                               world_vb.base_vertices );*/
	glBindVertexArray( world_vertex_buffer_.VAO_ );

	for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		for( int i= 0; i< world_vertex_buffer_.cluster_matrix_size_x_; i++ )
		{
			auto cluster= world_vertex_buffer_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ];
			/*glBindBuffer( GL_ARRAY_BUFFER, cluster->VBO_ );

			r_WorldVertex v;
			int shift;

			shift= ((char*)v.coord) - ((char*)&v);
			glEnableVertexAttribArray( 0 );
			glVertexAttribPointer( 0, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

			shift= ((char*)v.tex_coord) - ((char*)&v);
			glEnableVertexAttribArray( 1 );
			glVertexAttribPointer( 1, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

			shift= ((char*)&v.normal_id) - ((char*)&v);
			glEnableVertexAttribArray( 2 );
			glVertexAttribIPointer( 2, 1, GL_UNSIGNED_BYTE, sizeof(r_WorldVertex), (void*) shift );

			shift= ((char*)v.light) - ((char*)&v);
			glEnableVertexAttribArray( 3 );
			glVertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, sizeof(r_WorldVertex), (void*) shift );*/
			cluster->BindVBO();

			for( int y= 0; y< world_vertex_buffer_.chunks_per_cluster_y_; y++ )
				for( int x= 0; x< world_vertex_buffer_.chunks_per_cluster_x_; x++ )
				{
					auto ch= cluster->GetChunkVBOData( x, y );
					if( ch->vertex_count_ != 0 )
						glDrawElementsBaseVertex( GL_TRIANGLES, ch->vertex_count_ * 6 / 4, GL_UNSIGNED_SHORT, nullptr, ch->vbo_offset_ );
				}
		}
}

void r_WorldRenderer::DrawSky()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		false, false, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );
	r_OGLStateManager::UpdateState( state );

	skybox_shader_.Bind();
	skybox_shader_.Uniform( "cam_pos", cam_pos_ );
	skybox_shader_.Uniform( "view_matrix", view_matrix_ );

	skybox_vbo_.Bind();
	skybox_vbo_.Show();
}

void r_WorldRenderer::DrawSun()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		true, false, true, true,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );
	r_OGLStateManager::UpdateState( state );

	sun_texture_.Bind(0);

	sun_shader_.Bind();
	sun_shader_.Uniform( "view_matrix", view_matrix_ );
	sun_shader_.Uniform( "sun_vector", sun_vector_ );
	sun_shader_.Uniform( "cam_pos", cam_pos_ );
	sun_shader_.Uniform( "tex", 0 );

	glDrawArrays( GL_POINTS, 0, 1 );
}

void r_WorldRenderer::DrawWater()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );
	r_OGLStateManager::UpdateState( state );

	//water_texture.BindTexture( 0 );
	water_texture_.Bind(0);

	water_shader_.Bind();

	m_Mat4 water_matrix;
	water_matrix.Scale( m_Vec3( 1.0f, 1.0f, 1.0f/ float( R_WATER_VERTICES_Z_SCALER ) ) );
	water_final_matrix_= water_matrix * block_final_matrix_;
	water_shader_.Uniform( "view_matrix", water_final_matrix_ );
	water_shader_.Uniform( "tex", 0 );
	water_shader_.Uniform( "time", float( startup_time_.msecsTo(QTime::currentTime()) ) * 0.001f );

	water_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	water_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	water_shader_.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

	water_vb_.vbo.Bind();

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( -1.0f, -2.0f );
	glMultiDrawElementsBaseVertex(
		GL_TRIANGLES,
		water_vb_.chunk_meshes_index_count, GL_UNSIGNED_SHORT,//index count in every draw
		(void**)water_vb_.multi_indeces,// pointers to index data, must be 0
		water_vb_.quadchunks_to_draw,//number of draws
		water_vb_.base_vertices );//base vbertices

	glPolygonOffset( 0.0f, 0.0f );
	glDisable( GL_POLYGON_OFFSET_FILL );
}

void r_WorldRenderer::DrawBuildPrism()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		false, false, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );

	if( build_pos_.z < 0.0f )
		return;

	r_OGLStateManager::UpdateState( state );

	build_prism_shader_.Bind();
	build_prism_shader_.Uniform( "view_matrix", view_matrix_ );

	build_prism_shader_.Uniform( "build_prism_pos", build_pos_ );

	// m_Vec3 sh_cam_pos( cam_pos.x * H_SPACE_SCALE_VECTOR_X, cam_pos.y * H_SPACE_SCALE_VECTOR_Y
	build_prism_shader_.Uniform( "cam_pos", cam_pos_ );

	build_prism_vbo_.Bind();
	build_prism_vbo_.Show();
}

void r_WorldRenderer::BuildWorldWater()
{
	/*build quadchunk data structures*/
	quadchunk_num_x_= 1 + world_->ChunkNumberX()/2;
	//if( quadchunk_num_x * 2 != world_->ChunkNumberX() )
	//	quadchunk_num_x++;
	quadchunk_num_y_= 1 + world_->ChunkNumberY()/2;
	//if( quadchunk_num_y * 2 != world_->ChunkNumberY() )
	//	quadchunk_num_y++;
	water_quadchunk_info_= new r_WaterQuadChunkInfo[ quadchunk_num_x_ * quadchunk_num_y_ ];
	water_quadchunk_info_to_draw_= new r_WaterQuadChunkInfo[ quadchunk_num_x_ * quadchunk_num_y_ ];

	/*for( unsigned int i= 0; i< quadchunk_num_x; i++ )
	    for( unsigned int j= 0; j< quadchunk_num_y; j++ )
	    {
	        r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x ];
	        for( unsigned int n= 0; n< 2; n++ )
	            for( unsigned int m= 0; m< 2; m++ )
	            {
	                if( i*2 + n < chunk_num_x && j*2 + m < chunk_num_y )
	                    ch->chunks[n][m]= & chunk_info_[ i*2 + n + (j*2 + m) * chunk_num_x ];
	                else
	                    ch->chunks[n][m]= nullptr;
	                ch->water_updated= false;
	                ch->water_mesh_rebuilded= false;
	            }
	    }*/
	for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
		for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
		{
			r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
			ch->chunks_[0][0]= nullptr;
			ch->chunks_[0][1]= nullptr;
			ch->chunks_[1][0]= nullptr;
			ch->chunks_[1][1]= nullptr;
		}
	int first_quadchunk_longitude= world_->ChunkCoordToQuadchunkX( chunk_info_[0].chunk_->Longitude() );
	int first_quadchunk_latitude= world_->ChunkCoordToQuadchunkY( chunk_info_[0].chunk_->Latitude() );
	for( unsigned int i= 0; i< chunk_num_x_; i++ )
		for( unsigned int j= 0; j< chunk_num_y_; j++ )
		{
			r_ChunkInfo* ch= &chunk_info_[ i + j * chunk_num_x_ ];
			int quadchunk_longitude= ch->chunk_->Longitude() >>1;
			int quadchunk_latitude= ch->chunk_->Latitude() >>1;
			int chunk_local_quadchunk_coord[]= { ch->chunk_->Longitude()&1, ch->chunk_->Latitude()&1 };
			water_quadchunk_info_[ ( quadchunk_longitude - first_quadchunk_longitude ) +
								  quadchunk_num_x_ * ( quadchunk_latitude - first_quadchunk_latitude ) ]
				.chunks_[ ch->chunk_->Longitude()&1 ][ ch->chunk_->Latitude()&1 ]= ch;
		}

	/*build quadchunk data structures*/

	for( unsigned int i=0; i< chunk_num_x_; i++ )
		for( unsigned int j=0; j< chunk_num_y_; j++ )
		{
			chunk_info_[ i + j * chunk_num_x_ ].BuildWaterSurfaceMesh();
		}

	//allocate memory for vertices and build water quadchunk meshes
	unsigned int vertex_count= 0;
	for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
		for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
		{
			r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
			ch->GetVertexCount();
			ch->allocated_vertex_count_= ch->new_vertex_count_ + ( ch->new_vertex_count_ >>2 );//+ 25%
			vertex_count+= ch->allocated_vertex_count_;
		}

	water_vb_.allocated_vertex_count= vertex_count;
	water_vb_.vb_data= new r_WaterVertex[ vertex_count ];
	r_WaterVertex* v= water_vb_.vb_data;
	for( unsigned int i= 0; i< quadchunk_num_x_; i++ )
		for( unsigned int j= 0; j< quadchunk_num_y_; j++ )
		{
			r_WaterQuadChunkInfo* ch= &water_quadchunk_info_[ i + j * quadchunk_num_x_ ];
			ch->vb_data_= v;
			ch->BuildFinalMesh();
			v+= ch->allocated_vertex_count_;
		}
	//allocate memory for vertices and build water quadchunk meshes

	//generate index buffer for hexagons
	water_vb_.index_buffer_size= 65520;
	water_vb_.vb_index_data= new unsigned short[ water_vb_.index_buffer_size ];
	for( unsigned int i= 0, j= 0; i< water_vb_.index_buffer_size; i+=12, j+= 6 )
	{
		water_vb_.vb_index_data[i+0 ]= j+0;
		water_vb_.vb_index_data[i+1 ]= j+1;
		water_vb_.vb_index_data[i+2 ]= j+2;

		water_vb_.vb_index_data[i+3 ]= j+2;
		water_vb_.vb_index_data[i+4 ]= j+3;
		water_vb_.vb_index_data[i+5 ]= j+4;

		water_vb_.vb_index_data[i+6 ]= j+4;
		water_vb_.vb_index_data[i+7 ]= j+5;
		water_vb_.vb_index_data[i+8 ]= j+0;

		water_vb_.vb_index_data[i+9 ]= j+0;
		water_vb_.vb_index_data[i+10]= j+2;
		water_vb_.vb_index_data[i+11]= j+4;
	}

	water_vb_.vbo_update_ready= false;
	water_vb_.need_update_vbo= false;
	memcpy( water_quadchunk_info_to_draw_, water_quadchunk_info_,
			sizeof( r_WaterQuadChunkInfo ) * quadchunk_num_x_ * quadchunk_num_y_ );

	water_vb_.chunk_meshes_index_count= new int[ quadchunk_num_x_ * quadchunk_num_y_ ];
	water_vb_.base_vertices= new int[ quadchunk_num_x_ * quadchunk_num_y_ ];
	water_vb_.multi_indeces= new int*[ quadchunk_num_x_ * quadchunk_num_y_ ];
}

void r_WorldRenderer::BuildWorld()
{
	chunk_num_x_= world_->ChunkNumberX();
	chunk_num_y_= world_->ChunkNumberY();

	// srcete structures of world vertex buffer
	{
		world_vertex_buffer_.chunks_per_cluster_x_= 8;
		world_vertex_buffer_.chunks_per_cluster_y_= 8;

		int min_longitude= m_Math::DivNonNegativeRemainder( world_->Longitude(), world_vertex_buffer_.chunks_per_cluster_x_ );
		int min_latitude=  m_Math::DivNonNegativeRemainder( world_->Latitude() , world_vertex_buffer_.chunks_per_cluster_y_ );

		int max_longitude= m_Math::DivNonNegativeRemainder( world_->Longitude()+chunk_num_x_-1, world_vertex_buffer_.chunks_per_cluster_x_ );
		int max_latitude=  m_Math::DivNonNegativeRemainder( world_->Latitude() +chunk_num_y_-1, world_vertex_buffer_.chunks_per_cluster_y_ );

		world_vertex_buffer_.longitude_= min_longitude * world_vertex_buffer_.chunks_per_cluster_x_;
		world_vertex_buffer_.latitude_ = min_latitude  * world_vertex_buffer_.chunks_per_cluster_y_;
		world_vertex_buffer_.cluster_matrix_size_x_= 1 + max_longitude - min_longitude;
		world_vertex_buffer_.cluster_matrix_size_y_= 1 + max_latitude  - min_latitude ;

		world_vertex_buffer_to_draw_= world_vertex_buffer_;
		for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		{
			for( int i= 0; i< world_vertex_buffer_.cluster_matrix_size_x_; i++ )
			{
				int k=  i + j * r_WorldVBO::MAX_CLUSTERS_;
				world_vertex_buffer_.clusters_[k]= new r_WorldVBO::r_WorldVBOCluster(
					min_longitude+ i*world_vertex_buffer_.chunks_per_cluster_x_ ,
					min_latitude+  j*world_vertex_buffer_.chunks_per_cluster_y_ , &world_vertex_buffer_ );

				world_vertex_buffer_to_draw_.clusters_[k]= new r_WorldVBO::r_WorldVBOCluster(
					min_longitude+ i*world_vertex_buffer_.chunks_per_cluster_x_ ,
					min_latitude+  j*world_vertex_buffer_.chunks_per_cluster_y_ , &world_vertex_buffer_ );
			}
		}
	}

	// fill chunk matrix, setup chunk neighbos
	unsigned int k;
	for( unsigned int i=0; i< chunk_num_x_; i++ )
		for( unsigned int j=0; j< chunk_num_y_; j++ )
		{
			k= j * world_->ChunkNumberX() + i;
			chunk_info_[k].chunk_= world_->GetChunk( i, j );
			if( i < world_->ChunkNumberX() - 1 )
				chunk_info_[k].chunk_right_= world_->GetChunk( i + 1, j );
			else chunk_info_[k].chunk_right_= nullptr;
			if( j< world_->ChunkNumberY() - 1 )
				chunk_info_[k].chunk_front_= world_->GetChunk( i, j + 1 );
			else chunk_info_[k].chunk_front_= nullptr;
			if( j > 0 )
				chunk_info_[k].chunk_back_= world_->GetChunk( i, j - 1 );
			else chunk_info_[k].chunk_back_= nullptr;
			if( j > 0 && i < world_->ChunkNumberX() - 1 )
				chunk_info_[k].chunk_back_right_= world_->GetChunk( i + 1, j - 1 );
			else chunk_info_[k].chunk_back_right_= nullptr;
		}

	// calculate VBO size for each chunk
	for( unsigned int i=0; i< chunk_num_x_; i++ )
	{
		for( unsigned int j=0; j< chunk_num_y_; j++ )
		{
			k= j * world_->ChunkNumberX() + i;
			chunk_info_[k].GetQuadCount();

			chunk_info_[k].chunk_vb_.allocated_vertex_count=
				chunk_info_[k].chunk_vb_.new_vertex_count +
				((chunk_info_[k].chunk_vb_.new_vertex_count>>2) & 0xFFFFFFFC);

			auto chunk_vb_data= world_vertex_buffer_.GetChunkDataForGlobalCoordinates( i + world_->Longitude(), j + world_->Latitude() );
			chunk_vb_data->vertex_count_= chunk_info_[k].chunk_vb_.new_vertex_count;
			chunk_vb_data->reserved_vertex_count_= chunk_info_[k].chunk_vb_.allocated_vertex_count;
		}
	}

	// calculate vbo size for clusters and allocate memory
	for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		for( int i= 0; i< world_vertex_buffer_.cluster_matrix_size_x_; i++ )
		{
			r_WorldVBO::r_WorldVBOCluster* cluster= world_vertex_buffer_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ];
			cluster->PrepareBufferSizes();
			cluster->vbo_data_= new r_WorldVertex[ cluster->vbo_vertex_count_ ];

			r_WorldVBO::r_WorldVBOCluster* cluster_to_draw= world_vertex_buffer_to_draw_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ];
			*cluster_to_draw= *cluster;
		}

	// build chunks meshes
	for( unsigned int i=0; i< chunk_num_x_; i++ )
	{
		for( unsigned int j=0; j< chunk_num_y_; j++ )
		{
			k= j * world_->ChunkNumberX() + i;

			auto chunk_vb_data= world_vertex_buffer_.GetChunkDataForGlobalCoordinates( i + world_->Longitude(), j + world_->Latitude() );
			auto cluster= world_vertex_buffer_.GetClusterForGlobalCoordinates( i + world_->Longitude(), j + world_->Latitude() );

			chunk_info_[k].chunk_vb_.vb_data= chunk_vb_data->vbo_offset_ + cluster->vbo_data_;
			chunk_info_[k].BuildChunkMesh();
		}
	}

	world_vb_.vbo_update_ready = true;

	memcpy( chunk_info_to_draw_, chunk_info_,
			sizeof( r_ChunkInfo ) * world_->ChunkNumberX() * world_->ChunkNumberY() );
	//chunk list data
	unsigned int c= world_->ChunkNumberX() * world_->ChunkNumberY();
	world_vb_.chunk_meshes_index_count= new int[ c ];
	world_vb_.base_vertices= new int[ c ];
	world_vb_.multi_indeces= new int*[ c ];

	//chunk list water data
	world_vb_.chunk_meshes_water_index_count= new int[c];
	world_vb_.base_water_vertices= new int[ c ];

	BuildWorldWater();
}

void r_WorldRenderer::InitGL()
{
	if( settings_.value( "antialiasing", 0 ).toInt() != 0 )
		glEnable( GL_MULTISAMPLE );
	//glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );


	LoadShaders();
	InitFrameBuffers();
	LoadTextures();

	text_manager_= new r_Text( /*"textures/fixedsys8x18.bmp"*//*"textures/DejaVuSansMono12.bmp"*/"textures/mono_font_sdf.tga" );
	text_manager_->SetViewport( viewport_width_, viewport_height_ );

	BuildWorld();
	InitVertexBuffers();
}

void r_WorldRenderer::LoadShaders()
{
	char define_str[128];

	if( !world_shader_.Load( "shaders/world_frag.glsl", "shaders/world_vert.glsl", nullptr ) )
		h_Console::Error( "wold shader not found" );

	world_shader_.SetAttribLocation( "coord", 0 );
	world_shader_.SetAttribLocation( "tex_coord", 1 );
	world_shader_.SetAttribLocation( "normal", 2 );
	world_shader_.SetAttribLocation( "light", 3 );
	sprintf( define_str, "TEX_SCALE_VECTOR vec3( %1.8f, %1.8f, %1.8f )",
			 0.25f / float( H_MAX_TEXTURE_SCALE ),
			 0.25f * sqrt(3.0f) / float( H_MAX_TEXTURE_SCALE ),
			 1.0f );
	world_shader_.Define( define_str );
	world_shader_.Create();

	if( !water_shader_.Load( "shaders/water_frag.glsl", "shaders/water_vert.glsl", nullptr ) )
		h_Console::Error( "water shader not found" );

	water_shader_.SetAttribLocation( "coord", 0 );
	water_shader_.SetAttribLocation( "light", 1 );
	water_shader_.Create();

	if( !build_prism_shader_.Load( "shaders/build_prism_frag.glsl", "shaders/build_prism_vert.glsl", "shaders/build_prism_geom.glsl" ) )
		h_Console::Error( "build prism shader not found" );
	build_prism_shader_.SetAttribLocation( "coord", 0 );
	build_prism_shader_.Create();

	if( !skybox_shader_.Load( "shaders/sky_frag.glsl", "shaders/sky_vert.glsl", nullptr ) )
		h_Console::Error( "skybox shader not found" );
	skybox_shader_.SetAttribLocation( "coord", 0 );
	skybox_shader_.Create();


	if( !sun_shader_.Load( "shaders/sun_frag.glsl",  "shaders/sun_vert.glsl", nullptr ) )
		h_Console::Error( "sun shader not found" );

	sprintf( define_str, "SUN_SIZE %3.0f", float( viewport_width_ ) * 0.1f );
	sun_shader_.Define( define_str );
	sun_shader_.Create();

	if( !console_bg_shader_.Load( "shaders/console_bg_frag.glsl", "shaders/console_bg_vert.glsl", "shaders/console_bg_geom.glsl" ) )
		h_Console::Error( "console bg shader not found" );
	console_bg_shader_.Create();


	if( !supersampling_final_shader_.Load( "shaders/supersampling_2x2_frag.glsl", "shaders/fullscreen_quad_vert.glsl", nullptr ) )
		h_Console::Error( "supersampling final shader not found" );
	supersampling_final_shader_.Create();
}

void r_WorldRenderer::InitFrameBuffers()
{
	supersampling_buffer_.Create(
		std::vector<r_FramebufferTexture::TextureFormat>{ r_FramebufferTexture::FORMAT_RGBA8 },
		r_FramebufferTexture::FORMAT_DEPTH24_STENCIL8,
		viewport_width_ * 2,
		viewport_height_ * 2 );
}

void r_WorldRenderer::InitVertexBuffers()
{
	/*world_vb.vbo.VertexData( world_vb.vb_data,
	                         sizeof( r_WorldVertex ) * world_vb.allocated_vertex_count,
	                         sizeof( r_WorldVertex ) );

	world_vb.vbo.IndexData(  world_vb.vb_index_data,
	                         sizeof(quint16) * world_vb.index_buffer_size,
	                         GL_UNSIGNED_SHORT, GL_TRIANGLES );

	r_WorldVertex v;
	unsigned int shift;
	shift= ((char*)v.coord) - ((char*)&v );
	world_vb.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
	shift= ((char*)v.tex_coord) - ((char*)&v );
	world_vb.vbo.VertexAttribPointer( 1, 3, GL_SHORT, false, shift );
	shift= ((char*)&v.normal_id) - ((char*)&v );

	world_vb.vbo.VertexAttribPointerInt( 2, 1, GL_UNSIGNED_BYTE, shift );
	shift= ((char*)v.light ) - ((char*)&v );
	world_vb.vbo.VertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, shift );*/


	water_vb_.vbo.VertexData( water_vb_.vb_data,
							 sizeof( r_WaterVertex ) * water_vb_.allocated_vertex_count,
							 sizeof( r_WaterVertex ) );
	water_vb_.vbo.IndexData( water_vb_.vb_index_data, sizeof( short ) * water_vb_.index_buffer_size,
							GL_UNSIGNED_SHORT, GL_TRIANGLES );
	unsigned int shift;
	r_WaterVertex wv;
	shift= ((char*)&wv.coord[0])- ((char*)&wv );
	water_vb_.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
	shift= ((char*)&wv.light[0])- ((char*)&wv );
	water_vb_.vbo.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, false, shift );

	/*
	    water_side_vb.vbo.VertexData( water_side_vb.vb_data.Data(),
									 sizeof( r_WaterVertex ) * water_side_vb.quad_count * 4,
									sizeof( r_WaterVertex ) );
		water_side_vb.vbo.IndexData( water_side_vb.index_data, sizeof(short) * water_side_vb.quad_count * 6,
									GL_UNSIGNED_SHORT, GL_TRIANGLES );
		shift= ((char*)&wv.coord[0])- ((char*)&wv );
	    water_side_vb.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
	    shift= ((char*)&wv.water_depth)- ((char*)&wv );
	    water_side_vb.vbo.VertexAttribPointer( 1, 3, GL_UNSIGNED_BYTE, true, shift );*/

	static const float build_prism[]=
	{
		0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,   3.0f, 1.0f, 0.0f,
		2.0f, 2.0f, 0.0f,  0.0f, 2.0f, 0.0f,  -1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f,  2.0f, 0.0f, 1.0f,   3.0f, 1.0f, 1.0f,
		2.0f, 2.0f, 1.0f,  0.0f, 2.0f, 1.0f,  -1.0f, 1.0f, 1.0f
	};
	static const unsigned short build_prism_indeces[]=
	{
		0,1, 1,2, 2,3, 3,4,  4,5,   5,0,
		6,7, 7,8, 8,9, 9,10, 10,11, 11,6,
		0,6, 1,7, 2,8, 3,9,  4,10,  5,11
	};
	// 0,7, 1,8, 2,9, 3,10, 4,11, 5,6//additional lines
	// 0,3, 1,4, 2,5, 6,9, 7,10, 8,11  };

	build_prism_vbo_.VertexData( build_prism, sizeof( build_prism ), 12 );
	build_prism_vbo_.IndexData(
		(unsigned int*)(build_prism_indeces),
		sizeof(build_prism_indeces),
		GL_UNSIGNED_SHORT, GL_LINES );
	build_prism_vbo_.VertexAttribPointer( 0, 3, GL_FLOAT, false, 0 );

	static const short sky_vertices[]=
	{
		256, 256, 256, -256, 256, 256,
		256, -256, 256, -256, -256, 256,
		256, 256, -256, -256, 256, -256,
		256, -256, -256, -256, -256, -256
	};
	static const unsigned short sky_indeces[]=
	{
		0, 1, 5,  0, 5, 4,
		0, 4, 6,  0, 6, 2,
		4, 5, 7,  4, 7, 6,//bottom
		0, 3, 1,  0, 2, 3, //top
		2, 7, 3,  2, 6, 7,
		1, 3, 7,  1, 7, 5
	};
	skybox_vbo_.VertexData( sky_vertices, sizeof(short) * 8 * 3, sizeof(short) * 3 );
	skybox_vbo_.IndexData( sky_indeces, sizeof(unsigned short) * 36, GL_UNSIGNED_SHORT, GL_TRIANGLES );
	skybox_vbo_.VertexAttribPointer( 0, 3, GL_SHORT, false, 0 );

	world_vertex_buffer_.InitCommonData();
	for( int j= 0; j< world_vertex_buffer_.cluster_matrix_size_y_; j++ )
		for( int i= 0; i< world_vertex_buffer_.cluster_matrix_size_x_; i++ )
		{
			r_WorldVBO::r_WorldVBOCluster* cluster= world_vertex_buffer_.clusters_[ i + j * r_WorldVBO::MAX_CLUSTERS_ ];
			cluster->LoadVertexBuffer();
		}

	weather_effects_particle_manager_.Create( 65536*2, m_Vec3(72.0f, 72.0f, 96.0f) );
}

void r_WorldRenderer::LoadTextures()
{
	texture_manager_.SetTextureSize(
		max( min( settings_.value( "texture_size", R_MAX_TEXTURE_RESOLUTION ).toInt(), R_MAX_TEXTURE_RESOLUTION ), R_MIN_TEXTURE_RESOLUTION ) );

	texture_manager_.SetFiltration( settings_.value( "filter_textures", false ).toBool() );
	texture_manager_.LoadTextures();

	r_ImgUtils::LoadTexture( &water_texture_, "textures/water2.tga" );
	r_ImgUtils::LoadTexture( &sun_texture_, "textures/sun.tga" );
	r_ImgUtils::LoadTexture( &console_bg_texture_, "textures/console_bg_normalized.png" );
}
