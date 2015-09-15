#pragma once
#include <mutex>
#include <ctime>

#include "../hex.hpp"
#include "../fwd.hpp"
#include "../ticks_counter.hpp"
#include "i_world_renderer.hpp"
#include "chunk_data_cache.hpp"

#include "framebuffer_texture.hpp"
#include "framebuffer.hpp"
#include "polygon_buffer.hpp"
#include "glsl_program.hpp"
#include "text.hpp"
#include "texture_manager.hpp"
#include "weather_effects_particle_manager.hpp"

#include "wvb.hpp"

#include "matrix.hpp"
#include "../math_lib/collection.hpp"

#pragma pack( push, 1 )

struct r_WorldVertex
{
	short coord[3];
	short tex_coord[3];
	unsigned char light[2];
	unsigned char normal_id;
	char reserved[1];
};//16b struct

struct r_WaterVertex
{
	short coord[3];
	unsigned char light[2];
};//8b struct

#pragma pack( pop )


class r_ChunkInfo
{
public:

	r_ChunkInfo();
	void GetQuadCount();
	void BuildWaterSurfaceMesh();
	void BuildWaterSideMesh();
	void BuildChunkMesh();

	struct
	{
		// pointer to external storage for vertices
		r_WorldVertex* vb_data= nullptr;
		unsigned int vertex_count= 0;
	} chunk_vb_;

	//geomentry up and down range borders. Used only for generation of center chunk blocks( not for border blocks )
	int max_geometry_height_, min_geometry_height_;

	m_Collection< r_WaterVertex > water_surface_mesh_vertices_;
	m_Collection< r_WaterVertex > water_side_mesh_vertices_;

	bool chunk_data_updated_, chunk_water_data_updated_;
	//bool chunk_mesh_rebuilded_;

	const h_Chunk* chunk_;
	const h_Chunk* chunk_front_, *chunk_right_, *chunk_back_right_, *chunk_back_;
};

typedef std::unique_ptr<r_ChunkInfo> r_ChunkInfoPtr;

class r_WaterQuadChunkInfo
{
public:
	void GetVertexCount();
	void BuildFinalMesh();
	void GetUpdatedState();// set up water_updated= true, if any of h_ChunkInfo::chunk_water_data_updated= true

	r_WaterVertex* vb_data_;// pointer in array in r_WorldRenderer::water_vb_::vb_data
	unsigned int allocated_vertex_count_, real_vertex_count_;
	unsigned int new_vertex_count_;

	bool water_updated_;
	bool water_mesh_rebuilded_;

	r_ChunkInfo* chunks_[2][2];
};

class r_WorldRenderer final : public r_IWorldRenderer
{
public:
	r_WorldRenderer( const h_SettingsPtr& settings, const h_WorldConstPtr& world );
	virtual ~r_WorldRenderer() override;

public: // r_IWorldRenderer
	virtual void Update() override;
	virtual void UpdateChunk( unsigned short, unsigned short ) override;
	virtual void UpdateChunkWater( unsigned short, unsigned short ) override;
	virtual void FullUpdate() override;

public:
	void Draw();

	void Init();
	void InitGL();

	void SetCamPos( const m_Vec3& p );
	void SetCamAng( const m_Vec3& a );
	void SetBuildPos( const m_Vec3& p );
	void SetViewportSize( unsigned int viewport_width, unsigned int viewport_height );

private:
	r_WorldRenderer& operator=( const r_WorldRenderer& other ) = delete;

	void LoadShaders();
	void LoadTextures();
	void InitFrameBuffers();
	void InitVertexBuffers();
	void BuildWorld();
	void BuildWorldWater();

	// Recalc pointers to parent h_Chunk and his neighbors.
	void UpdateChunkMatrixPointers();
	void MoveChunkMatrix( int longitude, int latitude );

	void UpdateGPUData();

	void CalculateMatrices();
	void CalculateLight();
	void BuildChunkList();

	void DrawBuildPrism();
	void DrawWorld();
	void DrawWater();
	void DrawSky();
	void DrawSun();
	void DrawConsole();

private:
	const h_SettingsPtr settings_;
	const h_WorldConstPtr world_;

	h_TicksCounter frames_counter_;
	h_TicksCounter updates_counter_;

	// Shaders
	r_GLSLProgram world_shader_, build_prism_shader_, water_shader_, skybox_shader_, sun_shader_, console_bg_shader_, supersampling_final_shader_;

	//VBO
	r_PolygonBuffer build_prism_vbo_;
	r_PolygonBuffer skybox_vbo_;

	struct
	{
		m_Vec3 current_sun_light;
		m_Vec3 current_fire_light;
		m_Vec3 sun_direction;
	} lighting_data_;

	//framebuffers
	unsigned viewport_width_, viewport_height_;
	r_Framebuffer supersampling_buffer_;

	//textures
	r_TextureManager texture_manager_;
	r_FramebufferTexture sun_texture_;
	r_FramebufferTexture water_texture_;
	r_FramebufferTexture console_bg_texture_;

	//matrices and vectors
	m_Mat4 view_matrix_, block_scale_matrix_, block_final_matrix_, water_final_matrix_;
	m_Vec3 cam_ang_, cam_pos_, build_pos_, sun_vector_;

	//unsigned int chunk_num_x_, chunk_num_y_;
	struct
	{
		std::vector< r_ChunkInfoPtr > chunk_matrix;
		// same as world size
		unsigned int matrix_size[2];
		// Longitude + latitude
		unsigned int matrix_position[2];
	} chunks_info_;

	std::unique_ptr<r_WVB> world_vertex_buffer_;
	std::mutex world_vertex_buffer_mutex_;

	//text out
	r_Text* text_manager_;

	r_WeatherEffectsParticleManager weather_effects_particle_manager_;

	time_t startup_time_;
};

inline void r_WorldRenderer::SetCamPos( const m_Vec3& p )
{
	cam_pos_= p;
}

inline void r_WorldRenderer::SetCamAng( const m_Vec3& a )
{
	cam_ang_= a;
}

inline void r_WorldRenderer::SetBuildPos( const m_Vec3& p )
{
	build_pos_= p;
}

inline void r_WorldRenderer::SetViewportSize( unsigned int viewport_width, unsigned int viewport_height )
{
	viewport_width_= viewport_width;
	viewport_height_= viewport_height;
}
