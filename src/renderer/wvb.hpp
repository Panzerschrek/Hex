#pragma once

#include <memory>
#include <vector>

#include "../hex.hpp"
#include "ph.h"

struct r_VertexFormat
{
	struct Attribute
	{
		enum class TypeInShader
		{
			Real,
			Integer
		};

		TypeInShader type;

		// GL_FLOAT, GL_SHORT, etc.
		GLenum input_type;
		unsigned int components; // 1, 2, 3, 4
		unsigned int offset;
		bool normalized; // for real type
	};

	std::vector<Attribute> attributes;
	// Size of vertex structure, put sizeof(MyVertex) here.
	unsigned int vertex_size;
};

struct r_WorldVBOClusterSegment
{
	r_WorldVBOClusterSegment();

	unsigned int first_vertex_index;
	unsigned int vertex_count;
	unsigned int capacity;

	// for GPU thread. Set true - CPU thread, set false - GPU thread.
	bool updated;
};

class r_WorldVBOCluster
{
public:
	r_WorldVBOCluster();

	std::vector<char> vertices_;

	// As r_WorldVBOClusterSegment::updated
	bool buffer_reallocated_;

	r_WorldVBOClusterSegment segments_[ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];
};

typedef std::shared_ptr<r_WorldVBOCluster> r_WorldVBOClusterPtr;

class r_WorldVBOClusterGPU
{
public:
	r_WorldVBOClusterGPU(
		const r_WorldVBOClusterPtr& cpu_cluster,
		const r_VertexFormat& vertex_format,
		GLuint index_buffer );

	~r_WorldVBOClusterGPU();

	void SynchroniseSegmentsInfo( unsigned int cluster_size_x, unsigned int cluster_size_y );
	void UpdateVBO( unsigned int cluster_size_x, unsigned int cluster_size_y );
	void BindVBO();

private:
	r_WorldVBOClusterGPU& operator=(const r_WorldVBOClusterGPU&)= delete;

	const std::weak_ptr< r_WorldVBOCluster > cluster_;

	GLuint VBO_;
	GLuint VAO_;
	const unsigned int vertex_size_;

public:
	bool buffer_reallocated_;
	r_WorldVBOClusterSegment segments_[ H_MAX_CHUNKS_IN_CLUSTER * H_MAX_CHUNKS_IN_CLUSTER ];
};

typedef std::unique_ptr<r_WorldVBOClusterGPU> r_WorldVBOClusterGPUPtr;

class r_WVB
{
public:
	r_WVB(
		unsigned int cluster_size_x, unsigned int cluster_size_y,
		unsigned int cluster_matrix_size_x, unsigned int cluster_matrix_size_y,
		std::vector<unsigned short> indeces,
		r_VertexFormat vertex_format );

	// Call in GPU thread. Returns index buffer, and, maybe, create it, if it not exist.
	GLuint GetIndexBuffer();

	// Call in CPU thread. Returns cluster for longitude and latitude.
	r_WorldVBOClusterPtr GetCluster( int longitude, int latitude );
	// Call in CPU thread. Returns cluster segment for chunk with longitude and latitude.
	r_WorldVBOClusterSegment& GetClusterSegment( int longitude, int latitude );

	// Call in CPU thread. Moves cpu matrix to position longitude, latitude.
	void MoveCPUMatrix( short longitude, short latitude );

	// Call in GPU thread.
	void UpdateGPUMatrix( short longitude, short latitude );

	const unsigned int cluster_size_[2];
	const unsigned int cluster_matrix_size_[2];

	std::vector< r_WorldVBOClusterPtr    > cpu_cluster_matrix_;
	short cpu_cluster_matrix_coord_[2]; // longitude + latitude

	std::vector< r_WorldVBOClusterGPUPtr > gpu_cluster_matrix_;
	short gpu_cluster_matrix_coord_[2];

	GLuint index_buffer_;
	const std::vector<unsigned short> indeces_;

	r_VertexFormat vertex_format_;
};
