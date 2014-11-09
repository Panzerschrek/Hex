#include "ph.h"

class r_WorldVertex;

//vertices of all world
class r_WorldVBO
{
private:

	//vertices of rectangle of chunks
	struct r_WorldVBOCluster
	{
		// data of single chunk
		struct r_ChunkVBOData
		{
			unsigned int vertex_count_; // if zero, chunk does not exist
			unsigned int new_vertex_count_; // vertex count after recalculation of chunk quad count, but before chunk building
			unsigned int reserved_vertex_count_;
			unsigned int vbo_offset_; // chunk_data= r_WorldVBOCluster::vbo_data_ + vbo_offset_
		};

		// matrix of chunks in cluster
		r_ChunkVBOData chunks_vbo_data_ 		[ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];
		r_ChunkVBOData chunks_vbo_data_to_draw_ [ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];

		// opengl VBO object
		GLuint VBO_;

		r_WorldVertex* vbo_data_;
		unsigned int vbo_vertex_count_;
		unsigned int longitude_, latitude_;
	};

	// size of cluster. Not constant, becouse size must be different in different situations ( like other dimensions )
	unsigned int chunks_per_cluster_x_, chunks_per_cluster_y_;

	r_WorldVBOCluster clusters[ (H_MAX_CHUNKS/H_MIN_CHUNKS_IN_CLUSTER + 2) * (H_MAX_CHUNKS/H_MIN_CHUNKS_IN_CLUSTER + 2) ];

	// opengl objects
	GLuint VAO_;
	GLuint index_buffer_;
};
