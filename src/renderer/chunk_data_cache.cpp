#include "chunk_data_cache.hpp"


r_ChunkDataCache::r_ChunkDataCache()
{
	memset( chunk_quad_count_matrix_, 0, sizeof(chunk_quad_count_matrix_) );
	memset( chunk_water_hex_count_matrix_, 0, sizeof(chunk_water_hex_count_matrix_) );
}

r_ChunkDataCache::~r_ChunkDataCache()
{
}

void r_ChunkDataCache::SaveChunkMatrix( const char* file_name, const unsigned short* matrix ) const
{
	FILE* f= fopen( file_name, "wb" );
	if( f == NULL )
		return;

	fwrite( matrix, 1, CHUNK_MATRIX_WIDTH_ * CHUNK_MATRIX_HEIGHT_, f );
	fclose(f);
}

void r_ChunkDataCache::LoadChunkMatrix( const char* file_name, unsigned short* matrix )
{
	FILE* f= fopen( file_name, "rb" );
	if( f == NULL )
		return;

	fread( matrix, 1, CHUNK_MATRIX_WIDTH_ * CHUNK_MATRIX_HEIGHT_, f );
	fclose(f);
}
