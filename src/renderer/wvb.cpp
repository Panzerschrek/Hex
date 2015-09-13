#include "wvb.hpp"

r_WorldVBOCluster::r_WorldVBOCluster( short longitude, short latitude )
	: longitude_( longitude ), latitude_( latitude )
	, buffer_reallocated_( true )
{
}

/*
----------------r_WorldVBOClusterGPU-----------
*/

r_WorldVBOClusterGPU::r_WorldVBOClusterGPU(
	const r_WorldVBOClusterPtr& cpu_cluster,
	const r_VertexFormat& vertex_format,
	GLuint index_buffer )
	: cluster_( cpu_cluster )
	, vertex_size_( vertex_format.vertex_size )
{
	// TODO - add thread check

	glGenVertexArrays( 1, &VAO_ );
	glBindVertexArray( VAO_ );

	unsigned int i= 0;
	for( const r_VertexFormat::Attribute& attribute : vertex_format.attrbutes )
	{
		glEnableVertexAttribArray(i);

		if( attribute.type == r_VertexFormat::Attribute::TypeInShader::Integer )
			glVertexAttribIPointer(
				i,
				attribute.components, attribute.input_type,
				vertex_format.vertex_size, (void*) attribute.offset );
		else
			glVertexAttribPointer(
				i,
				attribute.components, attribute.input_type, attribute.normalized,
				vertex_format.vertex_size, (void*) attribute.offset );

		i++;
	}

	glGenBuffers( 1, &VBO_ );
	glBindBuffer( GL_ARRAY_BUFFER, VBO_ );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer );
}

r_WorldVBOClusterGPU::~r_WorldVBOClusterGPU()
{
	// TODO - add thread check

	glDeleteBuffers( 1, &VBO_ );
	glDeleteVertexArrays( 1, &VAO_ );
}

void r_WorldVBOClusterGPU::SynchroniseSegmentsInfo(
	unsigned int cluster_size_x, unsigned int cluster_size_y )
{
	r_WorldVBOClusterPtr cluster= cluster_.lock();
	if( !cluster )
	{
		for( unsigned int i= 0; i < cluster_size_x * cluster_size_y; i++ )
			segments_[i].updated= false;
		buffer_reallocated_= false;
	}
	else
	{
		for( unsigned int i= 0; i < cluster_size_x * cluster_size_y; i++ )
		{
			segments_[i]= cluster->segments_[i];
			cluster->segments_[i].updated= false;
		}

		buffer_reallocated_= cluster->buffer_reallocated_;
		cluster->buffer_reallocated_= false;
	}
}

void r_WorldVBOClusterGPU::UpdateVBO(
	unsigned int cluster_size_x, unsigned int cluster_size_y )
{
	r_WorldVBOClusterPtr cluster= cluster_.lock();
	if( !cluster ) return;

	glBindVertexArray( VAO_ );

	if( buffer_reallocated_ )
	{
		if( cluster->vertices_.size() > 0 )
			glBufferData(
				GL_ARRAY_BUFFER,
				cluster->vertices_.size(), cluster->vertices_.data(),
				GL_STATIC_DRAW );
	}
	else
	{
		for( unsigned int i= 0; i < cluster_size_x * cluster_size_y; i++ )
		{
			if( segments_[i].updated && segments_[i].vertex_count > 0 )
			{
				unsigned int offset= segments_[i].first_vertex_index * vertex_size_;

				glBufferSubData(
					GL_ARRAY_BUFFER,
					offset,
					segments_[i].vertex_count * vertex_size_,
					cluster->vertices_.data() + offset );
			}
		}
	}

	// Clear update flags.
	buffer_reallocated_= false;
	for( unsigned int i= 0; i < cluster_size_x * cluster_size_y; i++ )
		segments_[i].updated= false;
}
