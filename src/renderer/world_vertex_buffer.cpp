#include "world_vertex_buffer.hpp"
#include "../math_lib/m_math.h"

#include "world_renderer.hpp"

r_WorldVBO::r_WorldVBOCluster::r_ChunkVBOData::r_ChunkVBOData()
	: vertex_count_(0)
	, reserved_vertex_count_(0)
	, vbo_offset_(0)
	, updated_(false), rebuilded_(false)

{
}

r_WorldVBO::r_WorldVBOCluster::r_WorldVBOCluster( int longitude, int latitude, r_WorldVBO* vbo )
	: common_vbo_data_(vbo)
	, VBO_(0)
	, vbo_data_(nullptr)
	, vbo_vertex_count_(0)
	, longitude_(longitude)
	, latitude_ (latitude )
	, need_reallocate_vbo_(false)
{
}

void r_WorldVBO::r_WorldVBOCluster::PrepareBufferSizes()
{
	int non_empty_chunk_count= 0;
	int vertex_buffer_size_in_non_empty_chunks= 0;

	// calculate size of not existing chunks
	for( int j= 0; j< common_vbo_data_->chunks_per_cluster_y_; j++ )
		for( int i= 0; i< common_vbo_data_->chunks_per_cluster_x_; i++ )
		{
			r_ChunkVBOData* ch= GetChunkVBOData( i, j );
			if( ch->vertex_count_ != 0 )
			{
				vertex_buffer_size_in_non_empty_chunks+= ch->reserved_vertex_count_;
				non_empty_chunk_count++;
			}
		}

	int reserved_vertex_count_in_empty_chunks= vertex_buffer_size_in_non_empty_chunks / non_empty_chunk_count;
	reserved_vertex_count_in_empty_chunks= reserved_vertex_count_in_empty_chunks * 5 / 4;

	// reserve size for not created chunks
	for( int j= 0; j< common_vbo_data_->chunks_per_cluster_y_; j++ )
		for( int i= 0; i< common_vbo_data_->chunks_per_cluster_x_; i++ )
		{
			r_ChunkVBOData* ch= GetChunkVBOData( i, j );
			if( ch->vertex_count_ == 0 )
				ch->reserved_vertex_count_= reserved_vertex_count_in_empty_chunks;
		}

	// calculate total size of buffer
	vbo_vertex_count_= vertex_buffer_size_in_non_empty_chunks
					   + reserved_vertex_count_in_empty_chunks *
					   ( common_vbo_data_->chunks_per_cluster_y_ * common_vbo_data_->chunks_per_cluster_x_ - non_empty_chunk_count );

	// calculate offsets
	int offset= 0;
	for( int j= 0; j< common_vbo_data_->chunks_per_cluster_y_; j++ )
		for( int i= 0; i< common_vbo_data_->chunks_per_cluster_x_; i++ )
		{
			r_ChunkVBOData* ch= GetChunkVBOData( i, j );
			ch->vbo_offset_= offset;
			offset+= ch->reserved_vertex_count_;
		}
}

void r_WorldVBO::r_WorldVBOCluster::LoadVertexBuffer()
{
	int real_vertex_count= 0, allocated_vertex_count= 0;

	// calculate occupancy of buffer
	for( int j= 0; j< common_vbo_data_->chunks_per_cluster_y_; j++ )
		for( int i= 0; i< common_vbo_data_->chunks_per_cluster_x_; i++ )
		{
			r_ChunkVBOData* ch= GetChunkVBOData( i, j );
			real_vertex_count+= ch->vertex_count_;
			allocated_vertex_count+= ch->reserved_vertex_count_;
		}

	if( VBO_ == 0 )
		glGenBuffers( 1, &VBO_ );

	glBindBuffer( GL_ARRAY_BUFFER, VBO_ );

	if( float(real_vertex_count) / float(allocated_vertex_count) < 0.5f )
	{
		// real occupancy of buffer too low, send data parts

		glBufferData( GL_ARRAY_BUFFER, sizeof(r_WorldVertex) * vbo_vertex_count_, NULL, GL_STATIC_DRAW );
		for( int j= 0; j< common_vbo_data_->chunks_per_cluster_y_; j++ )
			for( int i= 0; i< common_vbo_data_->chunks_per_cluster_x_; i++ )
			{
				r_ChunkVBOData* ch= GetChunkVBOData( i, j );
				if( ch->vertex_count_ != 0 )
					glBufferSubData( GL_ARRAY_BUFFER,
									 ch->vbo_offset_ * sizeof(r_WorldVertex),
									 ch->vertex_count_ * sizeof(r_WorldVertex),
									 vbo_data_ + ch->vbo_offset_ );
			}
	}
	else
		glBufferData( GL_ARRAY_BUFFER, sizeof(r_WorldVertex) * vbo_vertex_count_, vbo_data_, GL_STATIC_DRAW );

}


r_WorldVBO::r_WorldVBOCluster* r_WorldVBO::GetClusterForGlobalCoordinates( int longitude, int latitude )
{
	int i= m_Math::DivNonNegativeRemainder( longitude - longitude_, chunks_per_cluster_x_ );
	int j= m_Math::DivNonNegativeRemainder( latitude  - latitude_ , chunks_per_cluster_y_ );
	return clusters_[ i + j * MAX_CLUSTERS_ ];
}

r_WorldVBO::r_WorldVBOCluster::r_ChunkVBOData* r_WorldVBO::GetChunkDataForGlobalCoordinates ( int longitude, int latitude )
{
	int i=  m_Math::DivNonNegativeRemainder( longitude - longitude_, chunks_per_cluster_x_ );
	int j=  m_Math::DivNonNegativeRemainder( latitude  - latitude_ , chunks_per_cluster_y_ );
	int di= m_Math::ModNonNegativeRemainder( longitude - longitude_, chunks_per_cluster_x_ );
	int dj= m_Math::ModNonNegativeRemainder( latitude  - latitude_ , chunks_per_cluster_y_ );
	return clusters_[ i + j * MAX_CLUSTERS_ ]->GetChunkVBOData( di, dj );
}

void r_WorldVBO::InitVAO()
{

	glGenVertexArrays( 1, &VAO_ );
	glBindVertexArray( VAO_ );

	/*r_WorldVertex v;
	int shift;

	glGenBuffers( 1, &stub_VBO_ );
	glBindBuffer( GL_ARRAY_BUFFER, stub_VBO_ );
	glBufferData( GL_ARRAY_BUFFER, 64, NULL, GL_STATIC_DRAW );

	shift= ((char*)v.coord) - ((char*)&v);
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)v.tex_coord) - ((char*)&v);
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)v.light) - ((char*)&v);
	glEnableVertexAttribArray( 3 );
	glVertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)&v.normal_id) - ((char*)&v);
	glEnableVertexAttribArray( 2 );
	glVertexAttribIPointer( 2, 1, GL_UNSIGNED_BYTE, sizeof(r_WorldVertex), (void*) shift );*/
}

void r_WorldVBO::InitIndexBuffer()
{
	unsigned int max_index= 65535 - 6;
	unsigned int index_count= (max_index/6) * 4; // 6 indeces and 4 vertices per quad
	index_count= index_count - index_count % 6;
	unsigned short* quads_indeces= new unsigned short[ index_count ];

	for( unsigned int x= 0, y=0; x< index_count; x+=6, y+=4 )
	{
		quads_indeces[x  ]= y;
		quads_indeces[x+1]= y + 1;
		quads_indeces[x+2]= y + 2;
		quads_indeces[x+3]= y;
		quads_indeces[x+4]= y + 2;
		quads_indeces[x+5]= y + 3;
	}

	glGenBuffers( 1, &index_buffer_ );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_buffer_ );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned short), quads_indeces, GL_STATIC_DRAW );

	delete[] quads_indeces;
}

void r_WorldVBO::InitCommonData()
{
	InitVAO();
	InitIndexBuffer();
}

void r_WorldVBO::r_WorldVBOCluster::BindVBO()
{
	glBindBuffer( GL_ARRAY_BUFFER, VBO_ );

	r_WorldVertex v;
	int shift;

	shift= ((char*)v.coord) - ((char*)&v);
	glEnableVertexAttribArray( 0 );
	glVertexAttribPointer( 0, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)v.tex_coord) - ((char*)&v);
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 1, 3, GL_SHORT, false, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)&v.normal_id) - ((char*)&v);
	glEnableVertexAttribArray( 2 );
	glVertexAttribIPointer( 2, 1, GL_UNSIGNED_BYTE, sizeof(r_WorldVertex), (void*) shift );

	shift= ((char*)v.light) - ((char*)&v);
	glEnableVertexAttribArray( 3 );
	glVertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, sizeof(r_WorldVertex), (void*) shift );
}
