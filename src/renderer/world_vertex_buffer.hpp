#pragma once
#include "../hex.hpp"
#include "ph.h"

class r_WorldVertex;

//vertices of all world
class r_WorldVBO
{
public:

	static const constexpr unsigned int MAX_CLUSTERS_= (H_MAX_CHUNKS/H_MIN_CHUNKS_IN_CLUSTER + 2);
	static const constexpr float MIN_BUFFER_OCCUPANCY_FOR_FULL_UPLOADING_= 0.5f;


	//vertices of rectangle of chunks
	struct r_WorldVBOCluster
	{
		// data of single chunk
		struct r_ChunkVBOData
		{
			unsigned int vertex_count_; // if zero, chunk does not exist
			unsigned int reserved_vertex_count_;
			unsigned int vbo_offset_; // chunk_data= r_WorldVBOCluster::vbo_data_ + vbo_offset_

			// flags
			bool updated_; // chunk in world updated
			bool rebuilded_; // chunk mesh ready to loading on GPU

			r_ChunkVBOData();
		};

		r_WorldVBOCluster( int longitude, int latitude, r_WorldVBO* vbo );

		void PrepareBufferSizes();

		//get chunk data, relative beginning of chunk cluster
		r_ChunkVBOData* GetChunkVBOData( int i, int j )
		{
			return chunks_vbo_data_ + i + j * H_MAX_CHUNKS_IN_CLUSTER;
		}

		// bind VBO and setup vertex format
		void BindVBO();

		// loads vertex buffer to gpu
		void LoadVertexBuffer();

		// matrix of chunks in cluster
		r_ChunkVBOData chunks_vbo_data_ [ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];

		// pointer, to acces of common data ( for all chunk clusters )
		r_WorldVBO* common_vbo_data_;

		// opengl VBO object
		GLuint VBO_;

		r_WorldVertex* vbo_data_;
		unsigned int vbo_vertex_count_;
		unsigned int longitude_, latitude_;

		// cluster flags
		bool need_reallocate_vbo_;
	};


	r_WorldVBOCluster* GetClusterForGlobalCoordinates( int longitude, int latitude );
	r_WorldVBOCluster::r_ChunkVBOData* GetChunkDataForGlobalCoordinates( int longitude, int latitude );


	// size of cluster. Not constant, becouse size must be different in different situations ( like other dimensions )
	unsigned int chunks_per_cluster_x_, chunks_per_cluster_y_;
	// size of matrix of cluster. Can be different in different world coordinates (+-1)
	unsigned int cluster_matrix_size_x_, cluster_matrix_size_y_;
	// coordinates of beginning of cluster matrix
	int longitude_, latitude_;

	r_WorldVBOCluster* clusters_[ MAX_CLUSTERS_ * MAX_CLUSTERS_ ];



public:

	void InitVAO();
	void InitIndexBuffer();
	void InitCommonData();


	// opengl objects
	GLuint VAO_;
	GLuint index_buffer_;
	GLuint stub_VBO_;
};
