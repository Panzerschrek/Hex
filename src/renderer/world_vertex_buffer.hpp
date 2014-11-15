#include "ph.h"

class r_WorldVertex;

//vertices of all world
class r_WorldVBO
{
private:

	static const constexpr unsigned int MAX_CLUSTERS_= (H_MAX_CHUNKS/H_MIN_CHUNKS_IN_CLUSTER + 1);

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
		r_ChunkVBOData chunks_vbo_data_ [ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];

		// opengl VBO object
		GLuint VBO_;

		r_WorldVertex* vbo_data_;
		unsigned int vbo_vertex_count_;
		unsigned int longitude_, latitude_;
	};

	// size of cluster. Not constant, becouse size must be different in different situations ( like other dimensions )
	unsigned int chunks_per_cluster_x_, chunks_per_cluster_y_;

	r_WorldVBOCluster clusters[ MAX_CLUSTERS_ * MAX_CLUSTERS_ ];



public:

	void InitVAO()
	{
		r_WorldVertex v;
		int shift;

		glGenVertexArrays( 1, &VAO_ );
		glBindVertexArray( VAO_ );

		shift= ((char*)v.coord) - ((char*)&v);
		glEnableVertexAttribArray( 0 );
		glVertexAttribPointer( 0, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

		shift= ((char*)v.tex_coord) - ((char*)&v);
		glEnableVertexAttribArray( 1 );
		glVertexAttribPointer( 1, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

		shift= ((char*)v.light) - ((char*)&v);
		glEnableVertexAttribArray( 2 );
		glVertexAttribPointer( 2, 2, GL_UNSIGNED_BYTE, false, sizeof(r_WorldVertex), (void*) shift );

		shift= ((char*)&v.normal_id) - ((char*)&v);
		glEnableVertexAttribArray( 3 );
		glVertexAttribIPointer( 3, 1, GL_UNSIGNED_BYTE, sizeof(r_WorldVertex), (void*) shift );



	}

	void InitCommonData()
	{
		InitVAO();

		unsigned int max_index= 65535;
		unsigned int index_count= (max_index/6) * 4; // 6 indeces and 4 vertices per quad
		unsigned short* quads_indeces= new unsigned short[ index_count ];

		for( unsigned int x= 0, y=0; x< index_count; x+=6, y+=4 )
		{
			quads_indeces[x] = y;
			quads_indeces[x + 1] = y + 1;
			quads_indeces[x + 2] = y + 2;

			quads_indeces[x + 3] = y;
			quads_indeces[x + 4] = y + 2;
			quads_indeces[x + 5] = y + 3;
		}

		glGenBuffers( 1, &index_buffer_ );
		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer_ );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned short), quads_indeces, GL_STATIC_DRAW );

		delete[] quads_indeces;

	}


	// opengl objects
	GLuint VAO_;
	GLuint index_buffer_;
};
