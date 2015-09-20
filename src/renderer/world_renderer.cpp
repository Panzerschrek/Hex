#include <vector>
#include <thread>

#include "world_renderer.hpp"
#include "glcorearb.h"
#include "rendering_constants.hpp"
#include "../console.hpp"
#include "../settings.hpp"
#include "../settings_keys.hpp"
#include "ogl_state_manager.hpp"
#include "img_utils.hpp"
#include "../math_lib/m_math.h"
#include "../math_lib/assert.hpp"

#include "../world.hpp"

static const float g_build_prism_vertices[]=
{
	0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,   3.0f, 1.0f, 0.0f,
	2.0f, 2.0f, 0.0f,  0.0f, 2.0f, 0.0f,  -1.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 1.0f,  2.0f, 0.0f, 1.0f,   3.0f, 1.0f, 1.0f,
	2.0f, 2.0f, 1.0f,  0.0f, 2.0f, 1.0f,  -1.0f, 1.0f, 1.0f
};

static const unsigned short g_build_prism_indeces[]=
{
	0,1, 1,2, 2,3, 3, 4,  4, 5,  5,0,
	6,7, 7,8, 8,9, 9,10, 10,11, 11,6,
	0,6, 1,7, 2,8, 3, 9,  4,10,  5,11
};
// 0,7, 1,8, 2,9, 3,10, 4,11, 5,6//additional lines
// 0,3, 1,4, 2,5, 6,9, 7,10, 8,11  };

r_WorldRenderer::r_WorldRenderer(
	const h_SettingsPtr& settings,
	const h_WorldConstPtr& world )
	: settings_(settings)
	, world_(world)
	, startup_time_(clock())
{
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
	}

	// Create world vertex buffer
	{
		// TODO - profile this and select optimal cluster size.
		unsigned int cluster_size[2] { 3, 3 };

		// Quad indexation
		unsigned int quad_count= 65536 / 6 - 1;
		std::vector<unsigned short> indeces( quad_count * 6 );
		for( unsigned int i= 0, v= 0; i< quad_count * 6; i+= 6, v+= 4 )
		{
			indeces[i  ]= v;
			indeces[i+1]= v + 1;
			indeces[i+2]= v + 2;
			indeces[i+3]= v;
			indeces[i+4]= v + 2;
			indeces[i+5]= v + 3;
		}

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

		world_vertex_buffer_.reset(
			new r_WVB(
				cluster_size[0],
				cluster_size[1],
				world_->ChunkNumberX() / cluster_size[0] + 2,
				world_->ChunkNumberY() / cluster_size[1] + 2,
				std::move(indeces),
				std::move(format) ));
	}

	// Set actual world position.
	UpdateWorldPosition( world_->Longitude(), world_->Latitude() );
}

r_WorldRenderer::~r_WorldRenderer()
{
	//update_thread.killTimer(
	//update_thread.wait();
	//update_thread.exit();
}

void r_WorldRenderer::Update()
{
	const std::unique_lock<std::mutex> wb_lock( world_vertex_buffer_mutex_ );

	r_WVB* wvb= world_vertex_buffer_.get();

	// Scan chunks matrix, calculate quads count.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ];
		H_ASSERT( chunk_info_ptr );
		if( chunk_info_ptr->updated_ )
		{
			chunk_info_ptr->GetQuadCount();

			r_WorldVBOClusterPtr cluster=
				wvb->GetCluster(
					chunks_info_.matrix_position[0] + x,
					chunks_info_.matrix_position[1] + y );

			r_WorldVBOClusterSegment& segment=
				wvb->GetClusterSegment(
					chunks_info_.matrix_position[0] + x,
					chunks_info_.matrix_position[1] + y );

			segment.vertex_count= chunk_info_ptr->vertex_count_;
			if( segment.vertex_count > segment.capacity )
				cluster->buffer_reallocated_= true;
		}
	}

	// Scan cluster matrix, find fully-updated.
	{
		std::vector< r_WorldVBOClusterPtr >& cluster_matrix= wvb->cpu_cluster_matrix_;
		for( unsigned int cy= 0; cy < wvb->cluster_matrix_size_[1]; cy++ )
		for( unsigned int cx= 0; cx < wvb->cluster_matrix_size_[0]; cx++ )
		{
			r_WorldVBOClusterPtr& cluster= cluster_matrix[ cx + cy * wvb->cluster_matrix_size_[0] ];
			if( cluster->buffer_reallocated_ )
			{
				// Scan chunks in cluster, calculate capacity for vertices.
				unsigned int vertex_count= 0;
				for( unsigned int y= 0; y < wvb->cluster_size_[1]; y++ )
				for( unsigned int x= 0; x < wvb->cluster_size_[0]; x++ )
				{
					r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wvb->cluster_size_[0] ];

					// Add 25% and round to 8up.
					segment.capacity= ( segment.vertex_count * 5 / 4 + 7 ) & (~7);
					vertex_count+= segment.capacity;
				}

				// Setup offsets and copy old data to new data.
				std::vector<char> new_vertices( vertex_count * sizeof(r_WorldVertex) );
				unsigned int offset= 0; // in vertices
				for( unsigned int y= 0; y < wvb->cluster_size_[1]; y++ )
				for( unsigned int x= 0; x < wvb->cluster_size_[0]; x++ )
				{
					r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wvb->cluster_size_[0] ];
					int longitude= int(x + cx * wvb->cluster_size_[0]) + wvb->cpu_cluster_matrix_coord_[0];
					int latitude = int(y + cy * wvb->cluster_size_[1]) + wvb->cpu_cluster_matrix_coord_[1];

					int cm_x= longitude - chunks_info_.matrix_position[0];
					int cm_y= latitude  - chunks_info_.matrix_position[1];
					r_ChunkInfo* chunk_info_ptr= nullptr;
					if( cm_x >= 0 && cm_x < chunks_info_.matrix_size[0] &&
						cm_y >= 0 && cm_y < chunks_info_.matrix_size[1] )
						chunk_info_ptr= chunks_info_.chunk_matrix[ cm_x + cm_y * chunks_info_.matrix_size[0] ].get();

					if( chunk_info_ptr && !chunk_info_ptr->updated_ )
					{
						//TODO - copying, now - only set update flag.
						chunk_info_ptr->updated_= true;
						if( chunk_info_ptr->vertex_data_ ) {}
						/*	memcpy(
							new_vertices.data() + offset * sizeof(r_WorldVertex),
							chunk_info_ptr->vertex_data_,
							segment.vertex_count * sizeof(r_WorldVertex) );*/
					}

					segment.first_vertex_index= offset;
					offset+= segment.capacity;
				}

				H_ASSERT( offset * sizeof(r_WorldVertex) == new_vertices.size() );
				cluster->vertices_= std::move( new_vertices );
			} // if cluster->buffer_reallocated_
		} // for clusters
	}

	// For statistics.
	unsigned int chunks_rebuilded= 0;

	// Scan chunks matrix, rebuild updated chunks.
	for( unsigned int y= 0; y < chunks_info_.matrix_size[1]; y++ )
	for( unsigned int x= 0; x < chunks_info_.matrix_size[0]; x++ )
	{
		r_ChunkInfoPtr& chunk_info_ptr= chunks_info_.chunk_matrix[ x + y * chunks_info_.matrix_size[0] ];
		H_ASSERT( chunk_info_ptr );
		if( chunk_info_ptr->updated_ )
		{
			chunks_rebuilded++;

			int longitude= chunks_info_.matrix_position[0] + (int)x;
			int latitude = chunks_info_.matrix_position[1] + (int)y;
			r_WorldVBOClusterSegment& segment=
				wvb->GetClusterSegment( longitude, latitude );
			r_WorldVBOClusterPtr cluster=
				wvb->GetCluster( longitude, latitude );

			chunk_info_ptr->vertex_data_=
				reinterpret_cast<r_WorldVertex*>(
				 cluster->vertices_.data() + segment.first_vertex_index * sizeof(r_WorldVertex) );
			chunk_info_ptr->BuildChunkMesh();

			// Finally, reset updated flag.
			chunk_info_ptr->updated_= false;
			// And set it in segment.
			segment.updated= true;
		}
	}

	// Not thread safe. writes here, reads in GPU thread.
	chunks_updates_counter_.Tick( chunks_rebuilded );
	updates_counter_.Tick();
}

void r_WorldRenderer::UpdateChunk(unsigned short X,  unsigned short Y )
{
	H_ASSERT( X >= 0 && X < chunks_info_.matrix_size[0] );
	H_ASSERT( Y >= 0 && Y < chunks_info_.matrix_size[1] );
	H_ASSERT( world_->Longitude() == chunks_info_.matrix_position[0] );
	H_ASSERT( world_->Latitude () == chunks_info_.matrix_position[1] );

	chunks_info_.chunk_matrix[ X + Y * chunks_info_.matrix_size[0] ]->updated_= true;
}

void r_WorldRenderer::UpdateChunkWater(unsigned short X,  unsigned short Y )
{
}

void r_WorldRenderer::UpdateWorldPosition( int longitude, int latitude )
{
	r_WVB* wvb= world_vertex_buffer_.get();

	// Move our chunk matrix.
	MoveChunkMatrix( longitude, latitude );

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
	lighting_data_.sun_direction= m_Vec3( 0.7f, 0.8f, 0.6f );
}

void r_WorldRenderer::Draw()
{
	UpdateGPUData();
	CalculateMatrices();
	CalculateLight();

	//supersampling_buffer.Bind();

	DrawWorld();
	DrawSky();
	DrawSun();
	//weather_effects_particle_manager_.Draw( view_matrix_, cam_pos_ );

	DrawBuildPrism();
	//DrawWater();

	/*r_Framebuffer::BindScreenFramebuffer();
	r_OGLStateManager::DisableBlend();
	r_OGLStateManager::DisableDepthTest();
	r_OGLStateManager::DepthMask(false);
	supersampling_final_shader.Bind();
	(*supersampling_buffer.GetTextures())[0].Bind(0);
	supersampling_final_shader.Uniform( "frame_buffer", 0 );
	glDrawArrays( GL_TRIANGLES, 0, 6 );*/

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
			"chunks: %dx%d\n", chunks_info_.matrix_size[0], chunks_info_.matrix_size[1] );
		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"quads: %d; per chunk: %d\n",
			world_quads_in_frame_,
			world_quads_in_frame_ / (chunks_info_.matrix_size[0] * chunks_info_.matrix_size[1]) );
		text_manager_->AddMultiText( 0, i++, text_scale, r_Text::default_color,
			"cam pos: %4.1f %4.1f %4.1f", cam_pos_.x, cam_pos_.y, cam_pos_.z );

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
	world_shader_.Uniform( "sun_vector", lighting_data_.sun_direction );
	world_shader_.Uniform( "view_matrix", block_final_matrix_ );

	world_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	world_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	world_shader_.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

	world_quads_in_frame_= 0;

	r_WVB* wvb= world_vertex_buffer_.get();
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
			// Not thread safe, because uses chunks_info_.matrix_position.
			int cm_x= int(x + cx * wvb->cluster_size_[0]) + wvb->gpu_cluster_matrix_coord_[0] - chunks_info_.matrix_position[0];
			int cm_y= int(y + cy * wvb->cluster_size_[1]) + wvb->gpu_cluster_matrix_coord_[1] - chunks_info_.matrix_position[1];
			if( cm_x < 0 || cm_x >= (int)chunks_info_.matrix_size[0] ||
				cm_y < 0 || cm_y >= (int)chunks_info_.matrix_size[1] )
				continue;

			r_WorldVBOClusterSegment& segment= cluster->segments_[ x + y * wvb->cluster_size_[0] ];
			if( segment.vertex_count > 0 )
			{
				glDrawElementsBaseVertex(
					GL_TRIANGLES,
					segment.vertex_count * 6 / 4,
					GL_UNSIGNED_SHORT,
					nullptr, segment.first_vertex_index );

				world_quads_in_frame_+= segment.vertex_count / 4;
			}
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
	sun_shader_.Uniform( "sun_vector", lighting_data_.sun_direction );
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

	water_texture_.Bind(0);

	water_shader_.Bind();

	m_Mat4 water_matrix;
	water_matrix.Scale( m_Vec3( 1.0f, 1.0f, 1.0f/ float( R_WATER_VERTICES_Z_SCALER ) ) );
	water_final_matrix_= water_matrix * block_final_matrix_;
	water_shader_.Uniform( "view_matrix", water_final_matrix_ );
	water_shader_.Uniform( "tex", 0 );
	water_shader_.Uniform( "time", float( (clock() - startup_time_) * 1000 / CLOCKS_PER_SEC) * 0.001f );

	water_shader_.Uniform( "sun_light_color", lighting_data_.current_sun_light );
	water_shader_.Uniform( "fire_light_color", lighting_data_.current_fire_light );
	water_shader_.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

	/*water_vb_.vbo.Bind();

	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( -1.0f, -2.0f );
	glMultiDrawElementsBaseVertex(
		GL_TRIANGLES,
		water_vb_.chunk_meshes_index_count, GL_UNSIGNED_SHORT,//index count in every draw
		(void**)water_vb_.multi_indeces,// pointers to index data, must be 0
		water_vb_.quadchunks_to_draw,//number of draws
		water_vb_.base_vertices );//base vbertices

	glPolygonOffset( 0.0f, 0.0f );
	glDisable( GL_POLYGON_OFFSET_FILL );*/
}

void r_WorldRenderer::DrawBuildPrism()
{
	static const GLenum state_blend_mode[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const float state_clear_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	static const r_OGLState state(
		true, false, true, false,
		state_blend_mode,
		state_clear_color,
		1.0f, GL_FRONT, GL_TRUE );

	if( build_pos_.z < 0.0f )
		return;

	r_OGLStateManager::UpdateState( state );

	build_prism_shader_.Bind();
	build_prism_shader_.Uniform( "view_matrix", view_matrix_ );

	build_prism_shader_.Uniform( "build_prism_pos", build_pos_ );

	// Setup active build side alpha.
	float alpha[12];
	for( int i= 0; i < 12; i++ ) alpha[i]= 0.15f;
	switch( build_direction_ )
	{
		case DOWN:
			alpha[6]= alpha[7]= alpha[8]= alpha[9]= alpha[10]= alpha[11]= 1.0f;
			break;
		case UP:
			alpha[0]= alpha[1]= alpha[2]= alpha[3]= alpha[ 4]= alpha[ 5]= 1.0f;
			break;
		case BACK:
			alpha[4]= alpha[3]= alpha[ 9]= alpha[10]= 1.0f;
			break;
		case FORWARD:
			alpha[0]= alpha[1]= alpha[ 6]= alpha[ 7]= 1.0f;
			break;
		case BACK_LEFT:
			alpha[2]= alpha[3]= alpha[ 8]= alpha[ 9]= 1.0f;
			break;
		case FORWARD_LEFT:
			alpha[1]= alpha[2]= alpha[ 7]= alpha[ 8]= 1.0f;
			break;
		case BACK_RIGHT:
			alpha[4]= alpha[5]= alpha[10]= alpha[11]= 1.0f;
			break;
		case FORWARD_RIGHT:
			alpha[0]= alpha[5]= alpha[ 6]= alpha[11]= 1.0f;
			break;
		case DIRECTION_UNKNOWN:
			break;
	};

	build_prism_shader_.Uniform( "active_vertices", alpha, 12 );

	build_prism_shader_.Uniform( "cam_pos", cam_pos_ );

	build_prism_vbo_.Bind();
	build_prism_vbo_.Show();
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

			if( x < chunks_info_.matrix_size[0] - 1 )
				chunk_info_ptr->chunk_right_= world_->GetChunk( x + 1, y );
			else chunk_info_ptr->chunk_right_= nullptr;
			if( y < chunks_info_.matrix_size[1] - 1 )
				chunk_info_ptr->chunk_front_= world_->GetChunk( x, y + 1 );
			else chunk_info_ptr->chunk_front_= nullptr;
			if( y > 0 )
				chunk_info_ptr->chunk_back_= world_->GetChunk( x, y - 1 );
			else chunk_info_ptr->chunk_back_= nullptr;
			if( y > 0 && x < chunks_info_.matrix_size[0] - 1 )
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

void r_WorldRenderer::UpdateGPUData()
{
	std::unique_lock<std::mutex> lock(world_vertex_buffer_mutex_);

	r_WVB* wvb= world_vertex_buffer_.get();

	wvb->UpdateGPUMatrix( wvb->cpu_cluster_matrix_coord_[0], wvb->cpu_cluster_matrix_coord_[1] );

	for( r_WorldVBOClusterGPUPtr& cluster : wvb->gpu_cluster_matrix_ )
	{
		cluster->SynchroniseSegmentsInfo( wvb->cluster_size_[0], wvb->cluster_size_[1] );
		cluster->UpdateVBO( wvb->cluster_size_[0], wvb->cluster_size_[1] );
	}
}

void r_WorldRenderer::InitGL()
{
	if( settings_->GetInt( h_SettingsKeys::antialiasing )!= 0 )
		glEnable( GL_MULTISAMPLE );
	//glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );


	LoadShaders();
	InitFrameBuffers();
	LoadTextures();

	text_manager_= new r_Text( /*"textures/fixedsys8x18.bmp"*//*"textures/DejaVuSansMono12.bmp"*/"textures/mono_font_sdf.tga" );
	text_manager_->SetViewport( viewport_width_, viewport_height_ );

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

	sprintf( define_str, "SUN_SIZE %3.0f", float( viewport_height_ ) * 0.1f );
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

	weather_effects_particle_manager_.Create( 65536*2, m_Vec3(72.0f, 72.0f, 96.0f) );
}

void r_WorldRenderer::LoadTextures()
{
	texture_manager_.SetTextureSize(
		std::max( std::min( settings_->GetInt( h_SettingsKeys::texture_size, 512 ), R_MAX_TEXTURE_RESOLUTION ), R_MIN_TEXTURE_RESOLUTION ) );

	texture_manager_.SetFiltration( settings_->GetBool( h_SettingsKeys::filter_textures, true ) );
	texture_manager_.LoadTextures();

	r_ImgUtils::LoadTexture( &water_texture_, "textures/water2.tga" );
	r_ImgUtils::LoadTexture( &sun_texture_, "textures/sun.tga" );
	r_ImgUtils::LoadTexture( &console_bg_texture_, "textures/console_bg_normalized.png" );
}
