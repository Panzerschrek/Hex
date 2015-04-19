#pragma once
#include <mutex>

#include <QSettings>
#include <QTime>

#include "../hex.hpp"
#include "../fwd.hpp"
#include "i_world_renderer.hpp"
#include "chunk_data_cache.hpp"

#include "framebuffer_texture.hpp"
#include "framebuffer.hpp"
#include "polygon_buffer.hpp"
#include "glsl_program.hpp"
#include "text.hpp"
#include "texture_manager.hpp"
#include "weather_effects_particle_manager.hpp"

#include "world_vertex_buffer.hpp"

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
		r_WorldVertex* vb_data;//pointer to r_WorldRenderer::world_vb::vb_data
		unsigned int allocated_vertex_count, real_vertex_count;
		unsigned int new_vertex_count;
	} chunk_vb_;

	//geomentry up and down range borders. Used only for generation of center chunk blocks( not for border blocks )
	int max_geometry_height_, min_geometry_height_;

	m_Collection< r_WaterVertex > water_surface_mesh_vertices_;
	m_Collection< r_WaterVertex > water_side_mesh_vertices_;

	bool chunk_data_updated_, chunk_water_data_updated_;
	bool chunk_mesh_rebuilded_;

	const h_Chunk* chunk_;
	const h_Chunk* chunk_front_, *chunk_right_, *chunk_back_right_, *chunk_back_;
};

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

class r_WorldRenderer : public r_IWorldRenderer
{
public:
	r_WorldRenderer( const h_World* world );
	virtual ~r_WorldRenderer() override;

public: // r_IWorldRenderer
	virtual void UpdateChunk( unsigned short,  unsigned short ) override;
	virtual void UpdateChunkWater( unsigned short,  unsigned short ) override;
	virtual void FullUpdate() override;

public:
	void Update() override {UpdateFunc();}
	void Draw();

	void Init();
	void InitGL();

	void SetCamPos( const m_Vec3& p );
	void SetCamAng( const m_Vec3& a );
	void SetBuildPos( const m_Vec3& p );
	void SetViewportSize( unsigned int viewport_width, unsigned int viewport_height );

private:
	void LoadShaders();
	void LoadTextures();
	void InitFrameBuffers();
	void InitVertexBuffers();
	void BuildWorld();
	void BuildWorldWater();
	void UpdateFunc();
	void UpdateWorld();
	void UpdateWater();
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

	void CalculateFPS();

private:
	const h_World* world_;

	//perfomance metrics
	unsigned int update_count;
	unsigned int frame_count;
	unsigned int last_fps;
	unsigned int frames_in_last_second;
	unsigned int updade_ticks_per_second, update_ticks_in_last_second;
	QTime last_fps_time;
	unsigned int chunk_updates_per_second, chunk_updates_in_last_second;
	unsigned int chunks_rebuild_per_second, chunk_rebuild_in_last_second;
	unsigned int water_quadchunks_updates_per_second, water_quadchunks_updates_in_last_second;
	unsigned int water_quadchunks_rebuild_per_second, water_quadchunks_rebuild_in_last_second;

	// Shaders
	r_GLSLProgram world_shader, build_prism_shader, water_shader, skybox_shader, sun_shader, console_bg_shader, supersampling_final_shader;

	//VBO
	r_PolygonBuffer build_prism_vbo;
	r_PolygonBuffer skybox_vbo;

	struct
	{
		r_PolygonBuffer vbo;
		unsigned int allocated_vertex_count;
		r_WorldVertex* vb_data;//world vertex buffer in RAM
		r_WorldVertex* new_vb_data;
		quint16* vb_index_data;
		unsigned int index_buffer_size;//number of indeces

		bool need_update_vbo;
		bool vbo_update_ready;

		//chunk list to rendering:
		int* chunk_meshes_index_count, *base_vertices;
		int**  multi_indeces;//array of nulls
		unsigned int chunks_to_draw;

		//water chunks to rendering
		int* chunk_meshes_water_index_count, *base_water_vertices;

	} world_vb;

	r_WorldVBO world_vertex_buffer_, world_vertex_buffer_to_draw_;
	r_WorldVBO* draw_vertex_buffer_, *gen_vertex_buffer;

	struct
	{
		r_PolygonBuffer vbo;
		r_WaterVertex* vb_data;
		r_WaterVertex* new_vb_data;
		unsigned int allocated_vertex_count;

		unsigned short* vb_index_data;
		unsigned int index_buffer_size;

		bool need_update_vbo;
		bool vbo_update_ready;

		//quadchunk list to rendering:
		int* chunk_meshes_index_count, *base_vertices;
		int**  multi_indeces;//array of nulls
		unsigned int quadchunks_to_draw;
	} water_vb;

	struct
	{
		m_Collection< r_WaterVertex > vb_data;
		unsigned int quad_count;
		unsigned short* index_data;
		r_PolygonBuffer vbo;

	} water_side_vb;


	struct
	{
		m_Vec3 current_sun_light;
		m_Vec3 current_fire_light;
		m_Vec3 sun_direction;
	} lighting_data;

	//framebuffers
	unsigned viewport_width_, viewport_height_;
	r_Framebuffer supersampling_buffer;

	//textures
	r_TextureManager texture_manager;
	r_FramebufferTexture sun_texture;
	r_FramebufferTexture water_texture;
	r_FramebufferTexture console_bg_texture;

	//matrices and vectors
	m_Mat4 view_matrix, block_scale_matrix, block_final_matrix, water_final_matrix;
	m_Vec3 cam_ang, cam_pos, build_pos, sun_vector;

	unsigned int quadchunk_num_x, quadchunk_num_y;
	unsigned int chunk_num_x, chunk_num_y;
	r_ChunkInfo chunk_info[ H_MAX_CHUNKS*H_MAX_CHUNKS ];
	r_ChunkInfo chunk_info_to_draw[ H_MAX_CHUNKS* H_MAX_CHUNKS];
	r_WaterQuadChunkInfo* water_quadchunk_info;
	r_WaterQuadChunkInfo* water_quadchunk_info_to_draw;

	//text out
	r_Text* text_manager;

	r_WeatherEffectsParticleManager weather_effects_particle_manager;

	//h_Thread<r_WorldRenderer> update_thread;
	std::mutex host_data_mutex, gpu_data_mutex;
	QTime startup_time;

	//config variables here:
	QSettings settings;

};

inline void r_WorldRenderer::SetCamPos( const m_Vec3& p )
{
	cam_pos= p;
}

inline void r_WorldRenderer::SetCamAng( const m_Vec3& a )
{
	cam_ang= a;
}

inline void r_WorldRenderer::SetBuildPos( const m_Vec3& p )
{
	build_pos= p;
}

inline void r_WorldRenderer::SetViewportSize( unsigned int viewport_width, unsigned int viewport_height )
{
	viewport_width_= viewport_width;
	viewport_height_= viewport_height;
}
