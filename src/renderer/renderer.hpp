#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include "texture.h"
#include "func_declarations.h"
#include "polygon_buffer.h"
#include "glsl_program.h"
#include "text.hpp"
#include "texture_manager.hpp"

#include "../math_lib/matrix.h"

#include "../world.hpp"
#include "../thread.hpp"

#pragma pack( push, 1 )
struct r_WorldVertex
{
    qint16 coord[3];
    qint16 tex_coord[3];
    quint8 light[2];
    quint8 normal_id;
    qint8 reserved[1];
};//16b struct


struct r_WaterVertex
{
	qint16 coord[3];
	//unsigned char water_depth;
	unsigned char light[2];
};//8b struct

#pragma pack( pop )



class r_ChunkInfo
{
	public:

	r_ChunkInfo();
	void GetQuadCount();
	void BuildWaterSurfaceMesh();
	void BuildChunkMesh();

    struct
    {
        r_WorldVertex* vb_data;//pointer to r_Renderer::world_vb::vb_data
        unsigned int allocated_vertex_count, real_vertex_count;
        unsigned int new_vertex_count;

        r_WorldVertex* water_vb_data;//pointer to water quads - in end of buffer
        unsigned int water_vertex_count;

    }chunk_vb;

    m_Collection< r_WaterVertex > water_surface_mesh_vertices;
    m_Collection< r_WaterVertex > water_side_mesh_vertices;

    bool chunk_data_updated, chunk_water_data_updated;
    bool chunk_mesh_rebuilded;

    h_Chunk* chunk;
	h_Chunk* chunk_front, *chunk_right, *chunk_back_right, *chunk_back;
};


class r_WaterQuadChunkInfo
{
	public:
	void GetVertexCount();
	void BuildFinalMesh();
	void GetUpdatedState();// set up water_updated= true, if any of h_ChunkInfo::chunk_water_data_updated= true

	r_WaterVertex* vb_data;// pointer in array in r_Renderer::water_vb::vb_data
	unsigned int allocated_vertex_count, real_vertex_count;
	unsigned int new_vertex_count;

	bool water_updated;
	bool water_mesh_rebuilded;

	r_ChunkInfo* chunks[2][2];

};

class r_Renderer : public QObject
{
    Q_OBJECT
	public:

	r_Renderer( h_World* w );
	~r_Renderer(){}


	void Draw();

	void Init();
	void InitGL();

	void SetCamPos( m_Vec3 p );
	void SetCamAng( m_Vec3 a );
	void SetBuildPos( m_Vec3 p );
	void SetViewportSize( unsigned int v_x, unsigned int v_y );
	private:

	private slots:
    void UpdateChunk( unsigned short,  unsigned short );
    void UpdateChunkWater( unsigned short,  unsigned short );

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
	void BuildChunkList();

	void DrawBuildPrism();
	void DrawWorld();
	void DrawWater();
	void DrawSky();
	void DrawSun();

	void CalculateFPS();

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


	//shaders
	r_GLSLProgram world_shader, build_prism_shader, water_shader, skybox_shader, sun_shader;


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

	}world_vb;


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


	}water_vb;

	struct
	{
		m_Collection< r_WaterVertex > vb_data;
		unsigned int quad_count;
		unsigned short* index_data;
		r_PolygonBuffer vbo;

	}water_side_vb;

	//frame buffers
	unsigned viewport_x, viewport_y;

	//textures
	r_TextureManager texture_manager;
	r_Texture sun_texture;
	r_Texture water_texture;


	//matrices and vectors
	m_Mat4 view_matrix, block_scale_matrix, block_final_matrix, water_final_matrix;
	m_Vec3 cam_ang, cam_pos, build_pos, sun_vector;

	h_World* world;

	unsigned int quadchunk_num_x, quadchunk_num_y;
	unsigned int chunk_num_x, chunk_num_y;
	r_ChunkInfo* chunk_info;
	r_ChunkInfo* chunk_info_to_draw;
	r_WaterQuadChunkInfo* water_quadchunk_info;
	r_WaterQuadChunkInfo* water_quadchunk_info_to_draw;

	//text out
	r_Text* text_manager;


	h_Thread<r_Renderer> update_thread;
	QMutex host_data_mutex, gpu_data_mutex;
	QTime startup_time;

};


inline void r_Renderer::SetCamPos( m_Vec3 p )
{
	cam_pos= p;
}

inline void r_Renderer::SetCamAng( m_Vec3 a )
{
	cam_ang= a;
}

inline void r_Renderer::SetViewportSize( unsigned int v_x, unsigned int v_y )
{
	viewport_x= v_x;
	viewport_y= v_y;
}

inline void r_Renderer::SetBuildPos( m_Vec3 p )
{
    build_pos= p;
}

#endif//RENDERER_H
