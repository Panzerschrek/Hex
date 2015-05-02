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
	struct Cluster
	{
		// data of single chunk
		struct ChunkVBOData
		{
			ChunkVBOData();

			unsigned int vertex_count_; // if zero, chunk does not exist
			unsigned int reserved_vertex_count_;
			unsigned int vbo_offset_; // chunk_data= r_WorldVBOCluster::vbo_data_ + vbo_offset_

			// flags
			bool updated_; // chunk in world updated
			bool rebuilded_; // chunk mesh ready to loading on GPU
		};

		Cluster( int longitude, int latitude, r_WorldVBO* vbo );
		~Cluster();

		void PrepareBufferSizes();
		//get chunk data, relative beginning of chunk cluster
		ChunkVBOData* GetChunkVBOData( int i, int j );
		// bind VBO and setup vertex format
		void BindVBO();
		// loads vertex buffer to gpu
		void LoadVertexBuffer();

		// matrix of chunks in cluster
		ChunkVBOData chunks_vbo_data_ [ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];

		// pointer, to acces of common data ( for all chunk clusters )
		r_WorldVBO* common_vbo_data_;

		// opengl VBO object
		GLuint VAO_;
		GLuint VBO_;

		r_WorldVertex* vbo_data_;
		unsigned int vbo_vertex_count_;
		unsigned int longitude_, latitude_;

		// cluster flags
		bool need_reallocate_vbo_;
	};

	Cluster* GetClusterForGlobalCoordinates( int longitude, int latitude );
	Cluster::ChunkVBOData* GetChunkDataForGlobalCoordinates( int longitude, int latitude );

	// size of cluster. Not constant, becouse size must be different in different situations ( like other dimensions )
	unsigned int chunks_per_cluster_x_, chunks_per_cluster_y_;
	// size of matrix of cluster. Can be different in different world coordinates (+-1)
	unsigned int cluster_matrix_size_x_, cluster_matrix_size_y_;
	// coordinates of beginning of cluster matrix
	int longitude_, latitude_;

	Cluster* clusters_[ MAX_CLUSTERS_ * MAX_CLUSTERS_ ];

public:
	void InitIndexBuffer();

	GLuint index_buffer_;
	GLuint stub_VAO_;
};

inline r_WorldVBO::Cluster::ChunkVBOData* r_WorldVBO::Cluster::GetChunkVBOData( int i, int j )
{
	return chunks_vbo_data_ + i + j * H_MAX_CHUNKS_IN_CLUSTER;
}
