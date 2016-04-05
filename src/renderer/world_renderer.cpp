#include <cstring>
#include <thread>
#include <vector>

#include "world_renderer.hpp"
#include "glcorearb.h"
#include "rendering_constants.hpp"
#include "../console.hpp"
#include "../settings.hpp"
#include "../settings_keys.hpp"
#include "../time.hpp"
#include "ogl_state_manager.hpp"
#include "shaders_loading.hpp"
#include "img_utils.hpp"
#include "chunk_info.hpp"
#include "weather_effects_particle_manager.hpp"
#include "wvb.hpp"
#include "../math_lib/math.hpp"
#include "../math_lib/assert.hpp"

#include "../block_collision.hpp"
#include "../player.hpp"
#include "../world.hpp"

struct r_ClipPlane
{
	m_Vec3 n;
	float dist;
};

struct r_StarVertex
{
	short pos[3];
	// Byte 0 - brightness
	// Byte 1 - spectre class. 0 - brown dwarf, 255 - blue giant
	unsigned char brightness_spectre[2];
};

static_assert( sizeof(r_StarVertex) == 8, "Unexpected struct size" );

static const float g_build_prism_vertices[]=
{
	0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,   3.0f, 1.0f, 0.0f,
	2.0f, 2.0f, 0.0f,  0.0f, 2.0f, 0.0f,  -1.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,  2.0f, 0.0f, 1.0f,   3.0f, 1.0f, 1.0f,
	2.0f, 2.0f, 1.0f,  0.0f, 2.0f, 1.0f,  -1.0f, 1.0f, 1.0f
};

static const unsigned short g_build_prism_indeces[]=
{
	0,1, 1,2, 2,3, 3, 4,  4, 5,  5,0, // lower side
	6,7, 7,8, 8,9, 9,10, 10,11, 11,6, // upper side
	0,6, 1,7, 2,8, 3, 9,  4,10, 5,11, // lateral sides
	0,7,1,6,  1,8,2,7,  2,9,3,8,  3,10,4,9,  4,11,5,10,  5,6,0,11, //additional lines for lateral sides
	0,3,  1,4,  2,5, // lower side additional
	6,9, 7,10, 8,11, // upper side additional
};

static const char* const g_supersampling_antialiasing_key_value= "ss";
static const char* const g_depth_based_antialiasing_key_value= "depth_based";
static const char* const g_fast_approximate_antialiasing_key_value= "fxaa";

static std::vector<unsigned short> GetQuadsIndeces()
{
	unsigned int quad_count= 65536 / 6 - 1;
	std::vector<unsigned short> indeces( quad_count * 6 );

	for( unsigned int i= 0, v= 0; i< quad_count * 6; i+= 6, v+= 4 )
	{
		indeces[i+0]= v + 0; indeces[i+1]= v + 1; indeces[i+2]= v + 2;
		indeces[i+3]= v + 0; indeces[i+4]= v + 2; indeces[i+5]= v + 3;
	}

	return indeces;
}

static r_VertexFormat GetWorldVertexFormat()
{
	r_VertexFormat format;

	format.vertex_size= sizeof(r_WorldVertex);

	r_WorldVertex v;
	r_VertexFormat::Attribute attrib;

	attrib.type= r_VertexFormat::Attribute::TypeInShader::Real;
	attrib.input_type= GL_SHORT;
	attrib.components= 3;
	attrib.offset= (char*)v.coord - (char*)&v;
	attrib.normalized= false;
	format.attributes.push_back(attrib);

	attrib.type= r_VertexFormat::Attribute::TypeInShader::Real;
	attrib.input_type= GL_SHORT;
	attrib.components= 3;
	attrib.offset= (char*)v.tex_coord - (char*)&v;
	attrib.normalized= false;
	format.attributes.push_back(attrib);

	attrib.type= r_VertexFormat::Attribute::TypeInShader::Integer;
	attrib.input_type= GL_UNSIGNED_BYTE;
	attrib.components= 1;
	attrib.offset= (char*)&v.normal_id - (char*)&v;
	attrib.normalized= false;
	format.attributes.push_back(attrib);

	attrib.type= r_VertexFormat::Attribute::TypeInShader::Real;
	attrib.input_type= GL_UNSIGNED_BYTE;
	attrib.components= 2;
	attrib.offset= (char*)v.light - (char*)&v;
	attrib.normalized= false;
	format.attributes.push_back(attrib);

	return format;
}

r_WorldRenderer::r_WorldRenderer(
	const h_SettingsPtr& settings,
	const h_WorldConstPtr& world ,
	const h_PlayerConstPtr& player )
	: settings_(settings)
	, world_(world)
	, player_(player)
	, startup_time_(hGetTimeMS())
{
	const char* antialiasing= settings_->GetString( h_SettingsKeys::antialiasing );
	if( strcmp( antialiasing, g_supersampling_antialiasing_key_value ) == 0 )
	{
		antialiasing_= Antialiasing::SuperSampling2x2;
		pixel_size_= 2;
	}
	else if( std::strcmp( antialiasing, g_depth_based_antialiasing_key_value ) == 0 )
	{
		antialiasing_= Antialiasing::DepthBased;
		pixel_size_= 1;
	}
	else if( std::strcmp( antialiasing, g_fast_approximate_antialiasing_key_value ) == 0 )
	{
		antialiasing_= Antialiasing::FastApproximate;
		pixel_size_= 1;
	}
	else
	{
		antialiasing_= Antialiasing::Other;
		pixel_size_= 1;
	}

	// Init chunk matrix
	{
		chunks_info_.matrix_size[0]= world->ChunkNumberX();
		chunks_info_.matrix_size[1]= world->ChunkNumberY();
		chunks_info_.matrix_position[0]= world->Longitude();
		chunks_info_.matrix_position[1]= world->Latitude ();

		unsigned int chunk_count= chunks_info_.matrix_size[0] * chunks_info_.matrix_size[1];
		chunks_info_.chunk_matrix.resize( chunk_count );
		for( r_ChunkInfoPtr& chunk_info_ptr : chunks_info_.chunk_matrix )
			chunk_info_ptr.reset( new r_ChunkInfo() );

		UpdateChunkMatrixPointers();

		chunks_info_for_drawing_.chunks_visibility_matrix.resize( chunk_count );
	}

	// TODO - profile this and select optimal cluster size.
	unsigned int world_cluster_size[2] { 3, 3 };
	unsigned int world_water_cluster_size[2] { 5, 4 };
	// Create world vertex buffer
	{
		world_vertex_buffer_.reset(
			new r_WVB(
				world_cluster_size[0],
				world_cluster_size[1],
				world_->ChunkNumberX() / world_cluster_size[0] + 2,
				world_->ChunkNumberY() / world_cluster_size[1] + 2,
				GetQuadsIndeces(),
				GetWorldVertexFormat() ));
	}

	// Create World water vertex buffr
	{
		// Hex indexation
		unsigned int hex_count= 65536 / 12 - 1;
		std::vector<unsigned short> indeces( hex_count * 12 );
		for( unsigned int i= 0, v= 0; i< hex_count * 6; i+= 12, v+= 6 )
		{
			indeces[i+ 0]= v + 0; indeces[i+ 1]= v + 1; indeces[i+ 2]= v + 2;
			indeces[i+ 3]= v + 2; indeces[i+ 4]= v + 3; indeces[i+ 5]= v + 4;
			indeces[i+ 6]= v + 4; indeces[i+ 7]= v + 5; indeces[i+ 8]= v + 0;
			indeces[i+ 9]= v + 0; indeces[i+10]= v + 2; indeces[i+11]= v + 4;
		}

		r_VertexFormat format;
		format.vertex_size= sizeof(r_WaterVertex);

		r_WaterVertex v;
		r_VertexFormat::Attribute attrib;

		attrib.type= r_VertexFormat::Attribute::TypeInShader::Real;
		attrib.input_type= GL_SHORT;
		attrib.components= 3;
		attrib.offset= (char*)v.coord - (char*)&v;
		attrib.normalized= false;
		format.attributes.push_back(attrib);

		attrib.type= r_VertexFormat::Attribute::TypeInShader::Real;
		attrib.input_type= GL_UNSIGNED_BYTE;
		attrib.components= 2;
		attrib.offset= (char*)v.light - (char*)&v;
		attrib.normalized= false;
		format.attributes.push_back(attrib);

		world_water_vertex_buffer_.reset(
			new r_WVB(
				world_water_cluster_size[0],
				world_water_cluster_size[1],
				world_->ChunkNumberX() / world_water_cluster_size[0] + 2,
				world_->ChunkNumberY() / world_water_cluster_size[1] + 2,
				std::move(indeces),
				std::move(format) ));
	}

	// Set actual world position.
	UpdateWorldPosition( world_->Longitude(), world_->Latitude() );
}

r_WorldRenderer::~r_WorldRenderer()
{
}

void r_WorldRenderer::Update()
{
	const std::lock_guard<std::mutex> wb_lock( world_vertex_buffer_mutex_ );

	r_WVB* wvb= world_vertex_buffer_.get();
	r_WVB* wvb_water= world_water_vertex_buffer_.get();

	// Scan chunks matrix, calculate quads count.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ];
		H_ASSERT( chunk_info_ptr );

		if( chunk_info_ptr->update_requested_ || chunk_info_ptr->water_update_requested_ )
		{
			bool need_update_in_this_tick= NeedRebuildChunkInThisTick( x, y );
			if( need_update_in_this_tick )
			{
				chunk_info_ptr->updated_= chunk_info_ptr->updated_ || chunk_info_ptr->update_requested_;
				chunk_info_ptr->water_updated_= chunk_info_ptr->water_updated_ || chunk_info_ptr->water_update_requested_;

				chunk_info_ptr->update_requested_= false;
				chunk_info_ptr->water_update_requested_= false;
			}
		}

		int longitude= chunks_info_.matrix_position[0] + int(x);
		int latitude = chunks_info_.matrix_position[1] + int(y);

		if( chunk_info_ptr->updated_ )
		{
			chunk_info_ptr->GetQuadCount();

			r_WorldVBOCluster& cluster=
				wvb->GetCluster( longitude, latitude );
			r_WorldVBOClusterSegment& segment=
				wvb->GetClusterSegment( longitude, latitude );

			segment.vertex_count= chunk_info_ptr->vertex_count_;
			if( segment.vertex_count > segment.capacity )
				cluster.buffer_reallocated_= true;
		}
		if( chunk_info_ptr->water_updated_ )
		{
			chunk_info_ptr->GetWaterHexCount();

			r_WorldVBOCluster& cluster=
				wvb_water->GetCluster( longitude, latitude );
			r_WorldVBOClusterSegment& segment=
				wvb_water->GetClusterSegment( longitude, latitude );

			segment.vertex_count= chunk_info_ptr->water_vertex_count_;
			if( segment.vertex_count > segment.capacity )
				cluster.buffer_reallocated_= true;
		}
	} // for chunks in matrix

	// Scan cluster matrix, find fully-updated.
	for( unsigned int i= 0; i < 2; i++ )
	{
		// pass #0 - for world vertex buffer
		// pass #1 - for water
		r_WVB* wb= (i == 0) ? wvb : wvb_water;

		std::vector< r_WorldVBOClusterPtr >& cluster_matrix= wb->cpu_cluster_matrix_;
		for( unsigned int cy= 0; cy < wb->cluster_matrix_size_[1]; cy++ )
		for( unsigned int cx= 0; cx < wb->cluster_matrix_size_[0]; cx++ )
		{
			r_WorldVBOClusterPtr& cluster= cluster_matrix[ cx + cy * wb->cluster_matrix_size_[0] ];
			if( cluster->buffer_reallocated_ )
			{
				// Scan chunks in cluster, calculate capacity for vertices.
				unsigned int vertex_count= 0;
				for( unsigned int y= 0; y < wb->cluster_size_[1]; y++ )
				for( unsigned int x= 0; x < wb->cluster_size_[0]; x++ )
				{
					r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wb->cluster_size_[0] ];
					// Add 25% and round to 8up.
					segment.capacity= ( segment.vertex_count * 5 / 4 + 7 ) & (~7);
					vertex_count+= segment.capacity;
				}

				// Setup offsets and copy old data to new data.
				std::vector<char> new_vertices( vertex_count * wb->vertex_format_.vertex_size );
				unsigned int offset= 0; // in vertices
				for( unsigned int y= 0; y < wb->cluster_size_[1]; y++ )
				for( unsigned int x= 0; x < wb->cluster_size_[0]; x++ )
				{
					r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wb->cluster_size_[0] ];
					int longitude= int(x + cx * wb->cluster_size_[0]) + wb->cpu_cluster_matrix_coord_[0];
					int latitude = int(y + cy * wb->cluster_size_[1]) + wb->cpu_cluster_matrix_coord_[1];

					int cm_x= longitude - chunks_info_.matrix_position[0];
					int cm_y= latitude  - chunks_info_.matrix_position[1];
					r_ChunkInfo* chunk_info_ptr= nullptr;
					if( cm_x >= 0 && cm_x < int(chunks_info_.matrix_size[0]) &&
						cm_y >= 0 && cm_y < int(chunks_info_.matrix_size[1]) )
						chunk_info_ptr= chunks_info_.chunk_matrix[ cm_x + cm_y * chunks_info_.matrix_size[0] ].get();

					if( i == 0 )
					{
						if( chunk_info_ptr && !chunk_info_ptr->updated_ &&
							segment.vertex_count != 0 )
						{
							H_ASSERT( segment.vertex_count == chunk_info_ptr->vertex_count_ );

							memcpy(
								new_vertices.data() + offset * sizeof(r_WorldVertex),
								cluster->vertices_.data() + segment.first_vertex_index * sizeof(r_WorldVertex),
								segment.vertex_count * sizeof(r_WorldVertex) );
						}
					}
					else
					{
						if( chunk_info_ptr && !chunk_info_ptr->water_updated_ &&
							segment.vertex_count != 0 )
						{
							H_ASSERT( segment.vertex_count == chunk_info_ptr->water_vertex_count_ );

							memcpy(
								new_vertices.data() + offset * sizeof(r_WaterVertex),
								cluster->vertices_.data() + segment.first_vertex_index * sizeof(r_WaterVertex),
								segment.vertex_count * sizeof(r_WaterVertex) );
						}
					}

					segment.first_vertex_index= offset;
					offset+= segment.capacity;
				}

				H_ASSERT( offset * wb->vertex_format_.vertex_size == new_vertices.size() );
				cluster->vertices_= std::move( new_vertices );
			} // if cluster->buffer_reallocated_
		} // for clusters
	} // for world and water vertex buffers

	// For statistics.
	unsigned int chunks_rebuilded= 0;
	unsigned int chunks_water_meshes_rebuilded= 0;

	// Scan chunks matrix, rebuild updated chunks.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ];
		H_ASSERT( chunk_info_ptr );
		int longitude= chunks_info_.matrix_position[0] + (int)x;
		int latitude = chunks_info_.matrix_position[1] + (int)y;

		if( chunk_info_ptr->updated_ )
		{
			chunks_rebuilded++;

			r_WorldVBOClusterSegment& segment=
				wvb->GetClusterSegment( longitude, latitude );
			r_WorldVBOCluster& cluster=
				wvb->GetCluster( longitude, latitude );

			chunk_info_ptr->vertex_data_=
				reinterpret_cast<r_WorldVertex*>(
				cluster.vertices_.data() + segment.first_vertex_index * sizeof(r_WorldVertex) );
			chunk_info_ptr->BuildChunkMesh();

			// Finally, reset updated flag.
			chunk_info_ptr->updated_= false;
			// And set it in segment.
			segment.updated= true;
		}
		if( chunk_info_ptr->water_updated_ )
		{
			chunks_water_meshes_rebuilded++;

			r_WorldVBOClusterSegment& segment=
				wvb_water->GetClusterSegment( longitude, latitude );
			r_WorldVBOCluster& cluster=
				wvb_water->GetCluster( longitude, latitude );

			chunk_info_ptr->water_vertex_data_=
				reinterpret_cast<r_WaterVertex*>(
				cluster.vertices_.data() + segment.first_vertex_index * sizeof(r_WaterVertex) );
			chunk_info_ptr->BuildWaterSurfaceMesh();

			// Finally, reset updated flag.
			chunk_info_ptr->water_updated_= false;
			// And set it in segment.
			segment.updated= true;
		}
	} // for chunks matrix

	BuildFailingBlocks();

	// Not thread safe. writes here, reads in GPU thread.
	chunks_updates_counter_.Tick( chunks_rebuilded );
	chunks_water_updates_counter_.Tick( chunks_water_meshes_rebuilded );
	updates_counter_.Tick();
}

void r_WorldRenderer::UpdateChunk(unsigned short X,  unsigned short Y, bool immediately )
{
	H_ASSERT( X < chunks_info_.matrix_size[0] );
	H_ASSERT( Y < chunks_info_.matrix_size[1] );
	H_ASSERT( world_->Longitude() == chunks_info_.matrix_position[0] );
	H_ASSERT( world_->Latitude () == chunks_info_.matrix_position[1] );

	unsigned int ind= X + Y * chunks_info_.matrix_size[0];
	if( immediately )
		chunks_info_.chunk_matrix[ ind ]->updated_= true;
	else
		chunks_info_.chunk_matrix[ ind ]->update_requested_= true;
}

void r_WorldRenderer::UpdateChunkWater(unsigned short X,  unsigned short Y, bool immediately )
{
	H_ASSERT( X < chunks_info_.matrix_size[0] );
	H_ASSERT( Y < chunks_info_.matrix_size[1] );
	H_ASSERT( world_->Longitude() == chunks_info_.matrix_position[0] );
	H_ASSERT( world_->Latitude () == chunks_info_.matrix_position[1] );

	unsigned int ind= X + Y * chunks_info_.matrix_size[0];
	if( immediately )
		chunks_info_.chunk_matrix[ ind ]->water_updated_= true;
	else
		chunks_info_.chunk_matrix[ ind ]->water_update_requested_= true;
}

void r_WorldRenderer::UpdateWorldPosition( int longitude, int latitude )
{
	const std::lock_guard<std::mutex> wb_lock( world_vertex_buffer_mutex_ );

	// Move our chunk matrix.
	MoveChunkMatrix( longitude, latitude );

	for( unsigned int i= 0; i< 2; i++ )
	{
		r_WVB* wvb= (i == 0) ? world_vertex_buffer_.get() : world_water_vertex_buffer_.get();

		// Calculate position of cluster matrix, move it.
		unsigned int cluster_matrix_coord[2]=
		{
			wvb->cluster_size_[0] *
			m_Math::DivNonNegativeRemainder(
				world_->Longitude(),
				wvb->cluster_size_[0] ),

			wvb->cluster_size_[1] *
			m_Math::DivNonNegativeRemainder(
				world_->Latitude (),
				wvb->cluster_size_[1] )
		};

		wvb->MoveCPUMatrix(
			cluster_matrix_coord[0],
			cluster_matrix_coord[1] );
	};
}

void r_WorldRenderer::CalculateMatrices()
{
	cam_pos_= player_->EyesPos();
	cam_ang_= player_->Angle();

	m_Mat4 translate, rotate_x, rotate_z, basis_change;

	fov_y_= m_Math::pi_2;
	fov_x_= 2.0f * std::atan( float(viewport_width_) / float(viewport_height_) * std::tan( fov_y_ * 0.5f ) );

	translate.Translate( -cam_pos_ );

	static const m_Vec3 s_vector(
		H_SPACE_SCALE_VECTOR_X / 3.0f,
		0.5f,
		1.0f );//hexogonal prism scale vector. DO NOT TOUCH!
	block_scale_matrix_.Scale( s_vector );

	// near clip plane does not clip nearest to player blocks
	float z_near=
		player_->MinEyesCollidersDistance() *
		std::cos( std::max( fov_x_, fov_y_ ) * 0.5f )
		* 0.95f;

	float z_far= 1024.0f;

	perspective_matrix_.PerspectiveProjection(
		float(viewport_width_)/float(viewport_height_),
		fov_y_,
		z_near,
		z_far );

	rotate_x.RotateX( -cam_ang_.x );
	rotate_z.RotateZ( -cam_ang_.z );

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	rotation_matrix_= rotate_z * rotate_x * basis_change * perspective_matrix_;
	view_matrix_= translate * rotation_matrix_;

	block_final_matrix_= block_scale_matrix_ * view_matrix_;
}

void r_WorldRenderer::CalculateLight()
{
	lighting_data_.sun_direction=
		world_->GetCalendar().GetSunVector( world_->GetTimeOfYear(), world_->GetGlobalWorldLatitude() );

	const float c_twilight_below_horizon= -0.1f;
	const float c_twilight_above_horizon= 0.05f;

	float daynight_k; // 0 - night, 1 - day
	if( lighting_data_.sun_direction.z > c_twilight_above_horizon )
		daynight_k= 1.0f;
	else if( lighting_data_.sun_direction.z < c_twilight_below_horizon)
		daynight_k= 0.0f;
	else
		daynight_k= (lighting_data_.sun_direction.z - c_twilight_below_horizon) / ( c_twilight_above_horizon - c_twilight_below_horizon );

	m_Vec3 sky_light=
		R_DAY_SKY_LIGHT_COLOR * daynight_k +
		R_NIGHT_SKY_LIGHT_COLOR * ( 1.0f - daynight_k );

	lighting_data_.current_sun_light= sky_light / float ( H_MAX_SUN_LIGHT * 16 );
	lighting_data_.current_fire_light= R_FIRE_LIGHT_COLOR / float ( H_MAX_FIRE_LIGHT * 16 );
	lighting_data_.current_ambient_light= R_AMBIENT_LIGHT_COLOR;

	lighting_data_.sky_color= R_SKYBOX_COLOR * daynight_k;
	lighting_data_.stars_brightness= 1.0f - daynight_k;

	if( player_->IsUnderwater() )
	{
		static const m_Vec3 c_underwater_color( 0.4f, 0.4f, 0.7f );

		lighting_data_.current_sun_light*= c_underwater_color;
		lighting_data_.current_fire_light*= c_underwater_color;
		lighting_data_.current_ambient_light*= c_underwater_color;
		lighting_data_.sky_color*= c_underwater_color;
		lighting_data_.stars_brightness*= ( c_underwater_color.x + c_underwater_color.y + c_underwater_color.z ) / 3.0f;
	}
}

void r_WorldRenderer::Draw()
{
	UpdateGPUData();
	CalculateMatrices();
	CalculateLight();
	CalculateChunksVisibility();

	rain_zone_heightmap_framebuffer_.Bind();
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	GenRainZoneHeightmap();

	bool draw_to_additional_framebuffer=
		antialiasing_ == Antialiasing::SuperSampling2x2 ||
		antialiasing_ == Antialiasing::DepthBased ||
		antialiasing_ == Antialiasing::FastApproximate;

	if( draw_to_additional_framebuffer )
	{
		additional_framebuffer_.Bind();
		glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	}
	else
		r_Framebuffer::BindScreenFramebuffer();

	DrawWorld();
	DrawSky();
	DrawStars();
	DrawSun();
	DrawRain();
	DrawWater();
	DrawBuildPrism();
	//DrawTestMob();

	if( draw_to_additional_framebuffer )
		r_Framebuffer::BindScreenFramebuffer();

	static const GLenum postprocessing_state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState postprocessing_state(
		false, false, false, false,
		postprocessing_state_blend_mode );

	if( antialiasing_ == Antialiasing::SuperSampling2x2 )
	{
		r_OGLStateManager::UpdateState( postprocessing_state );

		additional_framebuffer_.GetTextures()[0].Bind(0);

		supersampling_final_shader_.Bind();
		supersampling_final_shader_.Uniform( "frame_buffer", 0 );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}
	else if( antialiasing_ == Antialiasing::DepthBased  )
	{
		r_OGLStateManager::UpdateState( postprocessing_state );

		additional_framebuffer_.GetTextures()[0].Bind(0);
		additional_framebuffer_.GetDepthTexture().Bind(1);

		depth_based_antialiasing_shader_.Bind();
		depth_based_antialiasing_shader_.Uniform( "frame_buffer", 0 );
		depth_based_antialiasing_shader_.Uniform( "depth_buffer", 1 );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}
	else if( antialiasing_ == Antialiasing::FastApproximate )
	{
		r_OGLStateManager::UpdateState( postprocessing_state );

		additional_framebuffer_.GetTextures()[0].Bind(0);

		fast_approximate_antialiasing_shader_.Bind();
		fast_approximate_antialiasing_shader_.Uniform( "frame_buffer", 0 );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}

	DrawCrosshair();
	DrawConsole();

	if( settings_->GetBool( h_SettingsKeys::show_debug_info, false ) && h_Console::GetPosition() == 0.0f )
	{
		float text_scale= 0.25f;
		int i= 0;
		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"fps: %d", frames_counter_.GetTicksFrequency() );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"world ticks per second: %d", updates_counter_.GetTicksFrequency() );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"chunk updates per second: %d", chunks_updates_counter_.GetTicksFrequency() );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"chunk water updates per second: %d", chunks_water_updates_counter_.GetTicksFrequency() );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"chunks: %dx%d\n", chunks_info_.matrix_size[0], chunks_info_.matrix_size[1] );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"chunks visible: %d / %d", chunks_visible_, chunks_info_.matrix_size[0] * chunks_info_.matrix_size[1] );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"quads: %d; per chunk: %d\n",
			world_quads_in_frame_,
			world_quads_in_frame_ / (chunks_visible_ > 0 ? chunks_visible_ : 1) );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"water hexagons: %d; per chunk: %d\n",
			water_hexagons_in_frame_,
			water_hexagons_in_frame_ / (chunks_visible_ > 0 ? chunks_visible_ : 1) );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"cam pos: %4.1f %4.1f %4.1f cam ang: %1.2f %1.2f %1.2f",
			cam_pos_.x, cam_pos_.y, cam_pos_.z,
			cam_ang_.x, cam_ang_.y, cam_ang_.z );

		unsigned int ticks_in_day= world_->GetCalendar().GetTicksInDay();
		unsigned int day_ticks= world_->GetTimeOfYear() % ticks_in_day;

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"time: day %d %d:%d",
			world_->GetTimeOfYear() / ticks_in_day,
			day_ticks * 24 / ticks_in_day,
			day_ticks * 60 * 24 / ticks_in_day % 60 );

		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"sun z: %f", lighting_data_.sun_direction.z );

		//text_manager->AddMultiText( 0, 11, text_scale, r_Text::default_color, "quick brown fox jumps over the lazy dog\nQUICK BROWN FOX JUMPS OVER THE LAZY DOG\n9876543210-+/\\" );
		//text_manager->AddMultiText( 0, 0, 8.0f, r_Text::default_color, "#A@Kli\nO01-eN" );

		//text_manager->AddMultiText( 0, 1, 4.0f, r_Text::default_color, "QUICK BROWN\nFOX JUMPS OVER\nTHE LAZY DOG" );

		text_manager_->Draw();
	}

	frames_counter_.Tick();
}

void r_WorldRenderer::DrawConsole()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		false, false, true, false,
		state_blend_mode );

	h_Console::Move( 0.016f );

	if( h_Console::GetPosition() == 0.0f )
		return;

	r_OGLStateManager::UpdateState( state );

	console_bg_shader_.Bind();
	console_bg_shader_.Uniform( "tex", 0 );
	console_bg_shader_.Uniform( "pos", 1.0f - h_Console::GetPosition() );
	console_bg_shader_.Uniform( "screen_size", m_Vec3( viewport_width_, viewport_height_, 0.0f ) );

	console_bg_texture_.Bind(0);

	glDrawArrays( GL_POINTS, 0, 1 );
	h_Console::Draw( text_manager_.get() );
	text_manager_->Draw();
}

void r_WorldRenderer::CalculateChunksVisibility()
{
	m_Vec3 rel_cam_pos=
		cam_pos_ -
		m_Vec3(
			float(chunks_info_for_drawing_.matrix_position[0] * H_CHUNK_WIDTH) * H_SPACE_SCALE_VECTOR_X,
			float(chunks_info_for_drawing_.matrix_position[1] * H_CHUNK_WIDTH),
			0.0f);

	m_Mat4 normals_mat, rotate_x_mat, rotate_z_mat;
	rotate_x_mat.RotateX( cam_ang_.x );
	rotate_z_mat.RotateZ( cam_ang_.z );
	normals_mat= rotate_x_mat * rotate_z_mat;

	r_ClipPlane clip_planes[5];
	r_ClipPlane* plane;

	// near clip plane
	plane= &clip_planes[0];
	plane->n.x= 0.0f;
	plane->n.y= 1.0f;
	plane->n.z= 0.0f;
	plane->n= plane->n * normals_mat;
	plane->dist= -( plane->n * rel_cam_pos );
	// upper
	plane= &clip_planes[1];
	plane->n.x= 0.0f;
	plane->n.y= +std::sin( fov_y_ * 0.5f );
	plane->n.z= -std::cos( fov_y_ * 0.5f );
	plane->n= plane->n * normals_mat;
	plane->dist= -( plane->n * rel_cam_pos );
	// lower
	plane= &clip_planes[2];
	plane->n.x= 0.0f;
	plane->n.y= +std::sin( fov_y_ * 0.5f );
	plane->n.z= +std::cos( fov_y_ * 0.5f );
	plane->n= plane->n * normals_mat;
	plane->dist= -( plane->n * rel_cam_pos );
	// left
	plane= &clip_planes[3];
	plane->n.x= +std::cos( fov_x_ * 0.5f );
	plane->n.y= +std::sin( fov_x_ * 0.5f );
	plane->n.z= 0.0f;
	plane->n= plane->n * normals_mat;
	plane->dist= -( plane->n * rel_cam_pos );
	// right
	plane= &clip_planes[4];
	plane->n.x= -std::cos( fov_x_ * 0.5f );
	plane->n.y= +std::sin( fov_x_ * 0.5f );
	plane->n.z= 0.0f;
	plane->n= plane->n * normals_mat;
	plane->dist= -( plane->n * rel_cam_pos );

	chunks_visible_= 0;

	// TODO - optimize this.
	// Check bounding box of each chunk using 5 clip planes may b too expensive.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
	{
		m_Vec3 pos[2]=
		{
			m_Vec3(
				float(x * H_CHUNK_WIDTH) * H_SPACE_SCALE_VECTOR_X - 1.0f,
				float(y * H_CHUNK_WIDTH) - 1.0f,
				0.0f ),
			m_Vec3(
				float((x+1) * H_CHUNK_WIDTH) * H_SPACE_SCALE_VECTOR_X + 1.0f,
				float((y+1) * H_CHUNK_WIDTH) + 1.0f,
				float(H_CHUNK_HEIGHT) )
		};

		bool is_visible= true;
		for( unsigned int p= 0; p < sizeof(clip_planes) / sizeof(clip_planes[0]); p++ )
		{
			plane= &clip_planes[p];

			bool ahead_plane= false;
			for( unsigned int i= 0; i < 8; i++ )
			{
				m_Vec3 vertex_pos;
				vertex_pos.x= pos[ (i >> 0) & 1 ].x;
				vertex_pos.y= pos[ (i >> 1) & 1 ].y;
				vertex_pos.z= pos[ (i >> 2) & 1 ].z;

				if( plane->n * vertex_pos + plane->dist > 0.0f )
				{
					ahead_plane= true;
					break;
				}
			}
			if( !ahead_plane )
			{
				is_visible= false;
				break;
			}
		}

		chunks_visible_+= int(is_visible);
		chunks_info_for_drawing_.chunks_visibility_matrix[ x + y * chunks_info_.matrix_size[0] ]= is_visible;
	} // for chunks
}

unsigned int r_WorldRenderer::DrawClusterMatrix( r_WVB* wvb, unsigned int triangles_per_primitive, unsigned int vertices_per_primitive )
{
	unsigned int vertex_count= 0;

	for( unsigned int cy= 0; cy < wvb->cluster_matrix_size_[1]; cy++ )
	for( unsigned int cx= 0; cx < wvb->cluster_matrix_size_[0]; cx++ )
	{
		r_WorldVBOClusterGPUPtr& cluster= wvb->gpu_cluster_matrix_[ cx + cy * wvb->cluster_matrix_size_[0] ];
		if( !cluster ) continue;

		cluster->BindVBO();

		for( unsigned int y= 0; y < wvb->cluster_size_[1]; y++ )
		for( unsigned int x= 0; x < wvb->cluster_size_[0]; x++ )
		{
			// Reject invisible chunk in cluster.
			int cm_x=
				int(x + cx * wvb->cluster_size_[0]) + wvb->gpu_cluster_matrix_coord_[0] -
				chunks_info_for_drawing_.matrix_position[0];
			int cm_y=
				int(y + cy * wvb->cluster_size_[1]) + wvb->gpu_cluster_matrix_coord_[1] -
				chunks_info_for_drawing_.matrix_position[1];
			if( cm_x < 0 || cm_x >= (int)chunks_info_.matrix_size[0] ||
				cm_y < 0 || cm_y >= (int)chunks_info_.matrix_size[1] )
				continue;
			if ( !chunks_info_for_drawing_.chunks_visibility_matrix[ cm_x + cm_y * chunks_info_.matrix_size[0] ] )
				continue;

			r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wvb->cluster_size_[0] ];
			if( segment.vertex_count > 0 )
			{
				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					segment.vertex_count * 3 * triangles_per_primitive / vertices_per_primitive,
					GL_UNSIGNED_SHORT,
					nullptr, segment.first_vertex_index );

				vertex_count+= segment.vertex_count;
			}
		}
	}

	return vertex_count;
}

unsigned int r_WorldRenderer::DrawClusterMatrix(
	r_WVB* wvb,
	unsigned int triangles_per_primitive, unsigned int vertices_per_primitive,
	unsigned int start_chunk_x, unsigned int start_chunk_y,
	unsigned int   end_chunk_x, unsigned int   end_chunk_y )
{
	H_ASSERT( start_chunk_x <= end_chunk_x );
	H_ASSERT( start_chunk_y <= end_chunk_y );
	H_ASSERT( end_chunk_x < chunks_info_.matrix_size[0] );
	H_ASSERT( end_chunk_y < chunks_info_.matrix_size[1] );

	unsigned int vertex_count= 0;

	int dx= wvb->gpu_cluster_matrix_coord_[0] - chunks_info_for_drawing_.matrix_position[0];
	int dy= wvb->gpu_cluster_matrix_coord_[1] - chunks_info_for_drawing_.matrix_position[1];

	int cx_start= ( int(start_chunk_x) - dx ) / int(wvb->cluster_size_[0]);
	int cx_end  = ( int(  end_chunk_x) - dy ) / int(wvb->cluster_size_[0]);

	int cy_start= ( int(start_chunk_y) - dx ) / int(wvb->cluster_size_[1]);
	int cy_end  = ( int(  end_chunk_y) - dy ) / int(wvb->cluster_size_[1]);

	for( int cy= cy_start; cy <= cy_end; cy++ )
	for( int cx= cx_start; cx <= cx_end; cx++ )
	{
		r_WorldVBOClusterGPUPtr& cluster= wvb->gpu_cluster_matrix_[ cx + cy * wvb->cluster_matrix_size_[0] ];
		if( !cluster ) continue;

		cluster->BindVBO();

		for( int y= 0; y < int(wvb->cluster_size_[1]); y++ )
		for( int x= 0; x < int(wvb->cluster_size_[0]); x++ )
		{
			int cm_x= x + cx * int(wvb->cluster_size_[0]) + dx;
			int cm_y= y + cy * int(wvb->cluster_size_[1]) + dy;

			if( cm_x < int(start_chunk_x) || cm_x > int(end_chunk_x) ||
				cm_y < int(start_chunk_y) || cm_y > int(end_chunk_y) )
				continue;

			r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wvb->cluster_size_[0] ];
			if( segment.vertex_count > 0 )
			{
				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					segment.vertex_count * 3 * triangles_per_primitive / vertices_per_primitive,
					GL_UNSIGNED_SHORT,
					nullptr, segment.first_vertex_index );

				vertex_count+= segment.vertex_count;
			}
		}
	}

	return vertex_count;
}

void r_WorldRenderer::DrawWorld()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		false, true, true, false,
		state_blend_mode,
		r_OGLState::default_clear_color );
	r_OGLStateManager::UpdateState( state );

	texture_manager_.BindTextureArray( 0 );

	world_shader_.Bind();
	world_shader_.Uniform( "tex", 0 );

	m_Mat4 z_scale_mat;
	z_scale_mat.Scale( m_Vec3( 1.0f, 1.0f, 0.5f ) );

	world_shader_.Uniform( "view_matrix", z_scale_mat * block_final_matrix_ );

	world_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	world_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	world_shader_.Uniform( "ambient_light_color", lighting_data_.current_ambient_light );

	unsigned int vertex_count= DrawClusterMatrix( world_vertex_buffer_.get(), 2, 4 );
	world_quads_in_frame_= vertex_count / 4;

	// Failing blocks

	m_Mat4 z_scale_matrix;
	z_scale_matrix.Scale( m_Vec3( 1.0f, 1.0f, 1.0f / 256.0f ) );
	world_shader_.Uniform( "view_matrix", z_scale_matrix *  block_final_matrix_ );

	failing_blocks_vbo_.Bind();
	glDrawElements( GL_TRIANGLES, failing_blocks_vertex_count_ / 4 * 6, GL_UNSIGNED_SHORT, nullptr );
}

void r_WorldRenderer::DrawRain()
{
	weather_effects_particle_manager_->Draw(
		view_matrix_,
		cam_pos_,
		rain_zone_heightmap_framebuffer_.GetDepthTexture(),
		rain_zone_matrix_ );
}

void r_WorldRenderer::DrawSky()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		false, false, true, false,
		state_blend_mode,
		r_OGLState::default_clear_color );
	r_OGLStateManager::UpdateState( state );

	skybox_shader_.Bind();
	skybox_shader_.Uniform( "view_matrix", rotation_matrix_ );
	skybox_shader_.Uniform( "sky_color", lighting_data_.sky_color );

	skybox_vbo_.Bind();
	skybox_vbo_.Draw();
}

void r_WorldRenderer::DrawStars()
{
	if( lighting_data_.stars_brightness < 0.01f )
		return;

	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode );
	r_OGLStateManager::UpdateState( state );

	glPointSize( float(pixel_size_) );

	const h_Calendar& calendar= world_->GetCalendar();

	m_Mat4 sky_rotation_matrix;
	sky_rotation_matrix.Rotate(
		calendar.GetLocalRotationAxis( world_->GetGlobalWorldLatitude() ),
		calendar.GetSkySphereRotation( world_->GetTimeOfYear() ) );

	stars_shader_.Bind();
	stars_shader_.Uniform( "view_matrix", sky_rotation_matrix * rotation_matrix_ );
	stars_shader_.Uniform( "brightness", lighting_data_.stars_brightness );

	stars_vbo_.Bind();
	stars_vbo_.Draw();
}

void r_WorldRenderer::DrawSun()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, true,
		state_blend_mode,
		r_OGLState::default_clear_color );
	r_OGLStateManager::UpdateState( state );

	sun_texture_.Bind(0);

	sun_shader_.Bind();
	sun_shader_.Uniform( "view_matrix", rotation_matrix_ );
	sun_shader_.Uniform( "sun_vector", lighting_data_.sun_direction );
	sun_shader_.Uniform( "tex", 0 );

	glDrawArrays( GL_POINTS, 0, 1 );
}

void r_WorldRenderer::DrawWater()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		r_OGLState::default_clear_color );
	r_OGLStateManager::UpdateState( state );

	water_texture_.Bind(0);

	water_shader_.Bind();

	m_Mat4 water_matrix;
	water_matrix.Scale( m_Vec3( 1.0f, 1.0f, 1.0f / float( R_WATER_VERTICES_Z_SCALER ) ) );
	water_final_matrix_= water_matrix * block_final_matrix_;
	water_shader_.Uniform( "view_matrix", water_final_matrix_ );
	water_shader_.Uniform( "tex", 0 );
	water_shader_.Uniform( "time", float(hGetTimeMS() - startup_time_) * 0.001f );

	water_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	water_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	water_shader_.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

	unsigned int vertex_count= DrawClusterMatrix( world_water_vertex_buffer_.get(), 4, 6 );
	water_hexagons_in_frame_= vertex_count / 6;
}

void r_WorldRenderer::GenRainZoneHeightmap()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState world_state(
		false, true, true, false,
		state_blend_mode );
	static const r_OGLState water_state(
		false, false, true, false,
		state_blend_mode );

	rain_zone_heightmap_shader_.Bind();

	m_Mat4 world_scale_mat, water_scale_mat, scale_mat, translate_mat;

	world_scale_mat.Scale( m_Vec3( 1.0, 1.0f, 0.5f ) );
	water_scale_mat.Scale( m_Vec3( 1.0f, 1.0f, 1.0f / float(R_WATER_VERTICES_Z_SCALER) ) );

	scale_mat.Scale(
		m_Vec3(
			1.0f / R_RAIN_ZONE_RADIUS_M,
			1.0f / R_RAIN_ZONE_RADIUS_M,
			-1.0f / float(H_CHUNK_HEIGHT) ) );

	translate_mat.Translate( m_Vec3( -cam_pos_.xy(), 0.0f ) );
	rain_zone_matrix_= translate_mat * scale_mat;

	m_Vec2 zone_size( R_RAIN_ZONE_RADIUS_M + 1.5f, R_RAIN_ZONE_RADIUS_M + 1.5f );
	short start_coord[2];
	short   end_coord[2];
	pGetHexogonCoord( cam_pos_.xy() - zone_size, &start_coord[0], &start_coord[1] );
	pGetHexogonCoord( cam_pos_.xy() + zone_size, &  end_coord[0], &  end_coord[1] );
	for( unsigned int i= 0; i < 2; i++ )
	{
		start_coord[i]-= chunks_info_for_drawing_.matrix_position[i] << H_CHUNK_WIDTH_LOG2;
		  end_coord[i]-= chunks_info_for_drawing_.matrix_position[i] << H_CHUNK_WIDTH_LOG2;
	}

	int min_x= std::max( 0, start_coord[0] >> H_CHUNK_WIDTH_LOG2 );
	int max_x= std::min( int(chunks_info_.matrix_size[0]) - 1, end_coord[0] >> H_CHUNK_WIDTH_LOG2 );
	int min_y= std::max( 0, start_coord[1] >> H_CHUNK_WIDTH_LOG2 );
	int max_y= std::min( int(chunks_info_.matrix_size[1]) - 1, end_coord[1] >> H_CHUNK_WIDTH_LOG2 );

	rain_zone_heightmap_shader_.Uniform(
		"view_matrix",
		block_scale_matrix_ * world_scale_mat * rain_zone_matrix_ );

	r_OGLStateManager::UpdateState( world_state );

	DrawClusterMatrix(
		world_vertex_buffer_.get(), 2, 4,
		min_x, min_y,
		max_x, max_y );

	rain_zone_heightmap_shader_.Uniform(
		"view_matrix",
		block_scale_matrix_ * water_scale_mat * rain_zone_matrix_ );

	r_OGLStateManager::UpdateState( water_state );

	DrawClusterMatrix(
		world_water_vertex_buffer_.get(), 4, 6,
		min_x, min_y,
		max_x, max_y );
}

void r_WorldRenderer::DrawBuildPrism()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		r_OGLState::default_clear_color,
		r_OGLState::default_clear_depth,
		r_OGLState::default_cull_face_mode,
		false );

	if( player_->BuildDirection() == h_Direction::Unknown )
		return;

	r_OGLStateManager::UpdateState( state );

	build_prism_shader_.Bind();
	build_prism_shader_.Uniform( "view_matrix", view_matrix_ );

	build_prism_shader_.Uniform( "build_prism_pos", player_->BuildPos() );

	// Setup active build side alpha.
	float alpha[12];
	for( int i= 0; i < 12; i++ ) alpha[i]= 0.15f;
	switch( player_->BuildDirection() )
	{
		case h_Direction::Down:
			alpha[6]= alpha[7]= alpha[8]= alpha[9]= alpha[10]= alpha[11]= 1.0f;
			break;
		case h_Direction::Up:
			alpha[0]= alpha[1]= alpha[2]= alpha[3]= alpha[ 4]= alpha[ 5]= 1.0f;
			break;
		case h_Direction::Back:
			alpha[4]= alpha[3]= alpha[ 9]= alpha[10]= 1.0f;
			break;
		case h_Direction::Forward:
			alpha[0]= alpha[1]= alpha[ 6]= alpha[ 7]= 1.0f;
			break;
		case h_Direction::BackLeft:
			alpha[2]= alpha[3]= alpha[ 8]= alpha[ 9]= 1.0f;
			break;
		case h_Direction::ForwardLeft:
			alpha[1]= alpha[2]= alpha[ 7]= alpha[ 8]= 1.0f;
			break;
		case h_Direction::BackRight:
			alpha[4]= alpha[5]= alpha[10]= alpha[11]= 1.0f;
			break;
		case h_Direction::ForwardRight:
			alpha[0]= alpha[5]= alpha[ 6]= alpha[11]= 1.0f;
			break;
		case h_Direction::Unknown:
			break;
	};

	build_prism_shader_.Uniform( "active_vertices", alpha, 12 );
	build_prism_shader_.Uniform( "cam_pos", cam_pos_ );

	build_prism_vbo_.Bind();
	build_prism_vbo_.Draw();
}

void r_WorldRenderer::DrawTestMob()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		r_OGLState::default_clear_color,
		r_OGLState::default_clear_depth,
		r_OGLState::default_cull_face_mode,
		false );

	r_OGLStateManager::UpdateState( state );

	build_prism_shader_.Bind();
	build_prism_shader_.Uniform( "view_matrix", view_matrix_ );

	build_prism_shader_.Uniform( "build_prism_pos", world_->TestMobGetPosition() - m_Vec3(0.0f, 0.0f, 1.0f) );

	// Setup active build side alpha.
	float alpha[12];
	for( int i= 0; i < 12; i++ ) alpha[i]= 1.0f;

	build_prism_shader_.Uniform( "active_vertices", alpha, 12 );
	build_prism_shader_.Uniform( "cam_pos", cam_pos_ );

	build_prism_vbo_.Bind();
	build_prism_vbo_.Draw();
}

void r_WorldRenderer::DrawCrosshair()
{
	static const GLenum state_blend_mode[]= { GL_ONE, GL_ONE_MINUS_SRC_COLOR };
	static const r_OGLState state(
		true, false, false, false,
		state_blend_mode,
		r_OGLState::default_clear_color,
		r_OGLState::default_clear_depth,
		r_OGLState::default_cull_face_mode,
		false );

	r_OGLStateManager::UpdateState( state );

	crosshair_shader_.Bind();

	m_Mat4 mat;
	mat.Scale(
		m_Vec3(
			float(crosshair_texture_.Width ()) / float(viewport_width_ ),
			float(crosshair_texture_.Height()) / float(viewport_height_),
			1.0f));
	crosshair_shader_.Uniform( "transform_matrix", mat );

	crosshair_texture_.Bind(0);
	crosshair_shader_.Uniform( "tex", 0 );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
}

void r_WorldRenderer::UpdateChunkMatrixPointers()
{
	H_ASSERT( world_->Longitude() == chunks_info_.matrix_position[0] );
	H_ASSERT( world_->Latitude () == chunks_info_.matrix_position[1] );

	for( int y= 0; y < (int)chunks_info_.matrix_size[1]; y++ )
	for( int x= 0; x < (int)chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ];
		H_ASSERT( chunk_info_ptr );

		chunk_info_ptr->chunk_= world_->GetChunk( x, y );
		H_ASSERT( chunk_info_ptr->chunk_ );

		if( x < int(chunks_info_.matrix_size[0] - 1) )
			chunk_info_ptr->chunk_right_= world_->GetChunk( x + 1, y );
		else chunk_info_ptr->chunk_right_= nullptr;
		if( y < int(chunks_info_.matrix_size[1] - 1) )
			chunk_info_ptr->chunk_front_= world_->GetChunk( x, y + 1 );
		else chunk_info_ptr->chunk_front_= nullptr;
		if( y > 0 )
			chunk_info_ptr->chunk_back_= world_->GetChunk( x, y - 1 );
		else chunk_info_ptr->chunk_back_= nullptr;
		if( y > 0 && x < int(chunks_info_.matrix_size[0] - 1) )
			chunk_info_ptr->chunk_back_right_= world_->GetChunk( x + 1, y - 1 );
		else chunk_info_ptr->chunk_back_right_= nullptr;
	}
}

void r_WorldRenderer::MoveChunkMatrix( int longitude, int latitude )
{
	int dx= longitude - chunks_info_.matrix_position[0];
	int dy= latitude  - chunks_info_.matrix_position[1];

	if( dx == 0 && dy == 0 )
		return;

	std::vector< r_ChunkInfoPtr > new_matrix( chunks_info_.matrix_size[0] * chunks_info_.matrix_size[1] );

	for( int y= 0; y < (int)chunks_info_.matrix_size[1]; y++ )
	for( int x= 0; x < (int)chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= new_matrix[ x + y * chunks_info_.matrix_size[0] ];

		int old_x= x + dx;
		int old_y= y + dy;
		if( old_x >= 0 && old_x < (int)chunks_info_.matrix_size[0] &&
			old_y >= 0 && old_y < (int)chunks_info_.matrix_size[1] )
			chunk_info_ptr= std::move( chunks_info_.chunk_matrix[ old_x + old_y * chunks_info_.matrix_size[0] ] );
		else
			chunk_info_ptr.reset( new r_ChunkInfo() );
	}

	chunks_info_.chunk_matrix= std::move( new_matrix );
	chunks_info_.matrix_position[0]= longitude;
	chunks_info_.matrix_position[1]= latitude ;

	UpdateChunkMatrixPointers();
}

bool r_WorldRenderer::NeedRebuildChunkInThisTick( unsigned int x, unsigned int y )
{
	// Quad 9x9 - update every tick.
	const int c_full_update_radius= 4;
	// Quad 15x15 - update every 2 ticks.
	const int c_half_update_radius= 7;

	int dx= int(x) - int(chunks_info_.matrix_size[0] / 2);
	int dy= int(y) - int(chunks_info_.matrix_size[1] / 2);
	int r= std::max( std::abs(dx), std::abs(dy) );

	if( r <= c_full_update_radius ) return true;

	int longitude= int(x) + chunks_info_.matrix_position[0];
	int latitude = int(y) + chunks_info_.matrix_position[1];
	int ticks= (int) updates_counter_.GetTotalTicks();
	int b;

	if( r <= c_half_update_radius )
	{
		b= (longitude ^ latitude) & 1;
		return b == (ticks & 1);
	}

	b= (longitude&1) | ( (latitude &1) << 1 );
	return b == (ticks & 3);
}

void r_WorldRenderer::BuildFailingBlocks()
{
	failing_blocks_vertices_.clear();

	// Scan chunks matrix, rebuild updated chunks.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
		rBuildChunkFailingBlocks(
			*chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ],
			failing_blocks_vertices_ );
}

void r_WorldRenderer::UpdateGPUData()
{
	std::lock_guard<std::mutex> lock(world_vertex_buffer_mutex_);

	chunks_info_for_drawing_.matrix_position[0]= chunks_info_.matrix_position[0];
	chunks_info_for_drawing_.matrix_position[1]= chunks_info_.matrix_position[1];

	for( unsigned int i= 0; i < 2; i++ )
	{
		r_WVB* wvb= ( i == 0 ) ? world_vertex_buffer_.get() : world_water_vertex_buffer_.get();

		wvb->UpdateGPUMatrix( wvb->cpu_cluster_matrix_coord_[0], wvb->cpu_cluster_matrix_coord_[1] );

		for( r_WorldVBOClusterGPUPtr& cluster : wvb->gpu_cluster_matrix_ )
		{
			cluster->SynchroniseSegmentsInfo( wvb->cluster_size_[0], wvb->cluster_size_[1] );
			cluster->UpdateVBO( wvb->cluster_size_[0], wvb->cluster_size_[1] );
		}
	}

	failing_blocks_vbo_.VertexData( failing_blocks_vertices_.data(), failing_blocks_vertices_.size() * sizeof(r_WorldVertex), sizeof(r_WorldVertex) );
	failing_blocks_vertex_count_= failing_blocks_vertices_.size();
}

void r_WorldRenderer::InitGL( const h_LongLoadingCallback& long_loading_callback  )
{
	const float c_shaders_progress= 1.0f;
	const float c_framebuffers_progress= 1.0f;
	const float c_textures_progress= 20.0f;
	const float c_texts_progress= 2.0f;
	const float c_vertex_buffers_progress= 1.0f;
	float progress_scaler= 1.0f / (
		c_shaders_progress + c_framebuffers_progress +
		c_textures_progress + c_texts_progress );
	float progress= 0.0f;

	if( settings_->GetInt( h_SettingsKeys::antialiasing ) != 0 )
		glEnable( GL_MULTISAMPLE );
	//glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );

	LoadShaders();
	long_loading_callback( progress+= c_shaders_progress * progress_scaler );

	InitFrameBuffers();
	long_loading_callback( progress+= c_framebuffers_progress * progress_scaler );

	LoadTextures();
	long_loading_callback( progress+= c_textures_progress * progress_scaler );

	text_manager_.reset( new r_Text( "textures/mono_font_sdf.tga" ) );
	long_loading_callback( progress+= c_texts_progress * progress_scaler );

	InitVertexBuffers();
	long_loading_callback( progress+= c_vertex_buffers_progress * progress_scaler );
}

void r_WorldRenderer::LoadShaders()
{
	r_GLSLVersion glsl_version( r_GLSLVersion::v400 );

	char define_str[128];
	{
		std::snprintf(
			define_str, sizeof(define_str), "TEX_SCALE_VECTOR vec3( %1.8f, %1.8f, %1.8f )",
			1.0f / float( 4 * H_TEXTURE_SCALE_MULTIPLIER ),
			1.0f / float( 2 * H_TEXTURE_SCALE_MULTIPLIER ),
			1.0f );

		std::vector<std::string> defines;
		defines.emplace_back( define_str );
		if( settings_->GetBool( h_SettingsKeys::lighting_only ) ) defines.emplace_back( "LIGHTING_ONLY"  );

		world_shader_.ShaderSource(
			rLoadShader( "world_frag.glsl", glsl_version, defines ),
			rLoadShader( "world_vert.glsl", glsl_version, defines ) );
	}

	world_shader_.SetAttribLocation( "coord", 0 );
	world_shader_.SetAttribLocation( "tex_coord", 1 );
	// world_shader_.SetAttribLocation( "normal", 2 );
	world_shader_.SetAttribLocation( "light", 3 );
	world_shader_.Create();

	std::snprintf(
		define_str, sizeof(define_str), "TEX_SCALE_VECTOR vec2( %1.8f, %1.8f )",
		0.25f / 4.0f,
		0.25f / 2.0f );

	water_shader_.ShaderSource(
		rLoadShader( "water_frag.glsl", glsl_version ),
		rLoadShader( "water_vert.glsl", glsl_version, { define_str } ) );

	water_shader_.SetAttribLocation( "coord", 0 );
	water_shader_.SetAttribLocation( "light", 1 );
	water_shader_.Create();

	rain_zone_heightmap_shader_.ShaderSource(
		std::string(),
		rLoadShader( "rain_heightmap_vert.glsl", glsl_version ));
	rain_zone_heightmap_shader_.SetAttribLocation( "coord", 0 );
	rain_zone_heightmap_shader_.Create();

	build_prism_shader_.ShaderSource(
		rLoadShader( "build_prism_frag.glsl", glsl_version ),
		rLoadShader( "build_prism_vert.glsl", glsl_version ),
		rLoadShader( "build_prism_geom.glsl", glsl_version ) );
	build_prism_shader_.SetAttribLocation( "coord", 0 );
	build_prism_shader_.Create();

	skybox_shader_.ShaderSource(
		rLoadShader( "sky_frag.glsl", glsl_version ),
		rLoadShader( "sky_vert.glsl", glsl_version ) );
	skybox_shader_.SetAttribLocation( "coord", 0 );
	skybox_shader_.Create();

	stars_shader_.ShaderSource(
		rLoadShader( "stars_frag.glsl", glsl_version ),
		rLoadShader( "stars_vert.glsl", glsl_version ) );
	stars_shader_.SetAttribLocation( "coord", 0 );
	stars_shader_.SetAttribLocation( "birghtness_spectre", 1 );
	stars_shader_.Create();

	std::snprintf( define_str, sizeof(define_str), "SUN_SIZE %3.0f", float( viewport_height_ * pixel_size_ ) * 0.1f );
	sun_shader_.ShaderSource(
		rLoadShader( "sun_frag.glsl", glsl_version ),
		rLoadShader( "sun_vert.glsl", glsl_version, { define_str } ) );
	sun_shader_.Create();

	console_bg_shader_.ShaderSource(
		rLoadShader( "console_bg_frag.glsl", glsl_version ),
		rLoadShader( "console_bg_vert.glsl", glsl_version ),
		rLoadShader( "console_bg_geom.glsl", glsl_version ) );
	console_bg_shader_.Create();

	crosshair_shader_.ShaderSource(
		rLoadShader( "crosshair_frag.glsl", glsl_version ),
		rLoadShader( "crosshair_vert.glsl", glsl_version ) );
	crosshair_shader_.Create();

	supersampling_final_shader_.ShaderSource(
		rLoadShader( "supersampling_2x2_frag.glsl", glsl_version ),
		rLoadShader( "fullscreen_quad_vert.glsl", glsl_version ) );
	supersampling_final_shader_.Create();

	depth_based_antialiasing_shader_.ShaderSource(
		rLoadShader( "depth_based_antialiasing_frag.glsl", glsl_version ),
		rLoadShader( "fullscreen_quad_vert.glsl", glsl_version ) );
	depth_based_antialiasing_shader_.Create();

	fast_approximate_antialiasing_shader_.ShaderSource(
		rLoadShader( "fxaa_frag.glsl", glsl_version ),
		rLoadShader( "fullscreen_quad_vert.glsl", glsl_version ) );
	fast_approximate_antialiasing_shader_.Create();
}

void r_WorldRenderer::InitFrameBuffers()
{
	if( antialiasing_ == Antialiasing::SuperSampling2x2 )
	{
		additional_framebuffer_=
			r_Framebuffer(
				std::vector<r_Texture::PixelFormat>{ r_Texture::PixelFormat::RGBA8 },
				r_Texture::PixelFormat::Depth24Stencil8,
				viewport_width_  * 2,
				viewport_height_ * 2 );
	}
	else if( antialiasing_ == Antialiasing::DepthBased )
	{
		additional_framebuffer_=
			r_Framebuffer(
				std::vector<r_Texture::PixelFormat>{ r_Texture::PixelFormat::RGBA8 },
				r_Texture::PixelFormat::Depth24Stencil8,
				viewport_width_ ,
				viewport_height_ );
	}
	else if( antialiasing_ == Antialiasing::FastApproximate )
	{
		additional_framebuffer_=
			r_Framebuffer(
				std::vector<r_Texture::PixelFormat>{ r_Texture::PixelFormat::RGBA8 },
				r_Texture::PixelFormat::Depth24Stencil8,
				viewport_width_ ,
				viewport_height_ );

		additional_framebuffer_.GetTextures().front().SetFiltration(
			r_Texture::Filtration::Linear,
			r_Texture::Filtration::Linear );
	}

	rain_zone_heightmap_framebuffer_=
		r_Framebuffer(
			std::vector<r_Texture::PixelFormat>(),
			r_Texture::PixelFormat::Depth16,
			512,512 );

	rain_zone_heightmap_framebuffer_.GetDepthTexture().SetCompareMode(true);
}

void r_WorldRenderer::InitVertexBuffers()
{
	build_prism_vbo_.VertexData( g_build_prism_vertices, sizeof( g_build_prism_vertices ), sizeof(float) * 3 );
	build_prism_vbo_.IndexData(
		g_build_prism_indeces,
		sizeof(g_build_prism_indeces),
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
		4, 5, 7,  4, 7, 6, // bottom
		0, 3, 1,  0, 2, 3, // top
		2, 7, 3,  2, 6, 7,
		1, 3, 7,  1, 7, 5
	};
	skybox_vbo_.VertexData( sky_vertices, sizeof(short) * 8 * 3, sizeof(short) * 3 );
	skybox_vbo_.IndexData( sky_indeces, sizeof(unsigned short) * 36, GL_UNSIGNED_SHORT, GL_TRIANGLES );
	skybox_vbo_.VertexAttribPointer( 0, 3, GL_SHORT, false, 0 );

	{
		m_Rand randomizer;

		unsigned int stars_count= 2048;
		unsigned int i= 0;
		std::vector<r_StarVertex> stars(stars_count);

		{ // Make polar star
			m_Vec3 polar_direction= world_->GetCalendar().GetLocalRotationAxis( world_->GetGlobalWorldLatitude() );
			stars[0].pos[0]= short( polar_direction.x * 32767.0f );
			stars[0].pos[1]= short( polar_direction.y * 32767.0f );
			stars[0].pos[2]= short( polar_direction.z * 32767.0f );
			stars[0].brightness_spectre[0]= 255;
			stars[0].brightness_spectre[1]= 255;
			i++;
		}
		while( i < stars_count )
		{
			r_StarVertex& star= stars[i];
			star.pos[0]= randomizer.RandI( -32768, 32768 );
			star.pos[1]= randomizer.RandI( -32768, 32768 );
			star.pos[2]= randomizer.RandI( -32768, 32768 );
			unsigned int square_vec_length=
				((unsigned int) star.pos[0]) * ((unsigned int) star.pos[0]) +
				((unsigned int) star.pos[1]) * ((unsigned int) star.pos[1]) +
				((unsigned int) star.pos[2]) * ((unsigned int) star.pos[2]);
			if( square_vec_length > 32767u * 32767u ) continue;

			// Make more dark stars and less bright stars
			star.brightness_spectre[0]= (unsigned char) ( std::pow( randomizer.RandF(0.1f, 1.0f), 1.3f ) * 250.0f );
			// Make more brown dwarfs and less blue giants
			star.brightness_spectre[1]= (unsigned char) ( std::pow( randomizer.RandF(0.1f, 1.0f), 1.4f ) * 240.0f );

			i++;
		}

		stars_vbo_.VertexData( stars.data(), stars_count * sizeof(r_StarVertex), sizeof(r_StarVertex) );
		stars_vbo_.SetPrimitiveType( GL_POINTS );

		r_StarVertex v;
		stars_vbo_.VertexAttribPointer( 0, 3, GL_SHORT, true, ((char*)v.pos) - ((char*)&v) );
		stars_vbo_.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, true, ((char*)v.brightness_spectre) - ((char*)&v) );
	}

	{
		std::vector<unsigned short> indeces= GetQuadsIndeces();

		failing_blocks_vbo_.IndexData( indeces.data(), indeces.size() * sizeof(unsigned short), GL_UNSIGNED_SHORT, GL_TRIANGLES );
		failing_blocks_vbo_.VertexData( nullptr, 6 * sizeof(r_WorldVertex), sizeof(r_WorldVertex) );

		r_VertexFormat vertex_format= GetWorldVertexFormat();
		unsigned int i= 0;
		for( const r_VertexFormat::Attribute& attribute : vertex_format.attributes )
		{
			if( attribute.type == r_VertexFormat::Attribute::TypeInShader::Integer )
				failing_blocks_vbo_.VertexAttribPointerInt(
					i,
					attribute.components, attribute.input_type,
					attribute.offset );
			else
				failing_blocks_vbo_.VertexAttribPointer(
					i,
					attribute.components, attribute.input_type, attribute.normalized,
					attribute.offset );

			i++;
		}
	}

	unsigned int rain_particle_count= (unsigned int)(
		( R_RAIN_ZONE_RADIUS_M * 2.0f ) *
		( R_RAIN_ZONE_RADIUS_M * 2.0f ) *
		R_RAIN_ZONE_HEIGHT_M *
		R_RAIN_DENSITY );

	weather_effects_particle_manager_.reset(
		new r_WeatherEffectsParticleManager(
			rain_particle_count,
			m_Vec3(
				R_RAIN_ZONE_RADIUS_M * 2.0f,
				R_RAIN_ZONE_RADIUS_M * 2.0f,
				R_RAIN_ZONE_HEIGHT_M) ) );
}

void r_WorldRenderer::LoadTextures()
{
	unsigned int textures_detalization= settings_->GetInt( h_SettingsKeys::textures_detalization );

	texture_manager_.SetTextureDetalization( textures_detalization );
	texture_manager_.SetFiltration( settings_->GetBool( h_SettingsKeys::filter_textures, true ) );
	texture_manager_.LoadTextures();

	r_ImgUtils::LoadTexture( &water_texture_, "textures/water2.tga", textures_detalization );
	r_ImgUtils::LoadTexture( &sun_texture_, "textures/sun.tga" );
	r_ImgUtils::LoadTexture( &console_bg_texture_, "textures/console_bg_normalized.png" );
	r_ImgUtils::LoadTexture( &crosshair_texture_, "textures/crosshair.bmp" );
}
