#pragma once

#include <memory>
#include <vector>

#include "../hex.hpp"
#include "ph.h"

struct r_VertexFormat
{
	struct Component
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

	std::vector<Component> components;
	// size fo vertex structure, put sizeof(MyVertex) here
	unsigned int vertex_size;
};

struct r_WorldVBOClusterSegment
{
	unsigned int first_vertex_index;
	unsigned int vertex_count;
	unsigned int capacity;

	// for GPU thread. Set true - CPU thread, set false - GPU thread.
	bool updated;
};

struct r_WorldVBOCluster
{
	std::vector<char> vertices_;

	short longitude_, latitude_;

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

typedef std::shared_ptr<r_WorldVBOClusterGPU> r_WorldVBOClusterGPUPtr;

class r_WVB
{
	unsigned int cluster_matrix_size_[2];
	unsigned int cluster_size_[2];

	std::vector< r_WorldVBOClusterPtr    > cpu_cluster_matrix_;
	std::vector< r_WorldVBOClusterGPUPtr > gpu_cluster_matrix_;
};
