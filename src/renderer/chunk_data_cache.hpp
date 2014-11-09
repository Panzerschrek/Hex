#ifndef CHUNK_DATA_CACHE_HPP
#define CHUNK_DATA_CACHE_HPP

#include "../hex.hpp"
#include "ph.h"

class r_ChunkDataCache
{
	public:

	static const constexpr unsigned int CHUNK_MATRIX_WIDTH_= (1+H_MAX_LONGITUDE-H_MIN_LONGITUDE);
	static const constexpr unsigned int CHUNK_MATRIX_HEIGHT_= (1+H_MAX_LATITUDE-H_MIN_LATITUDE);

	r_ChunkDataCache();
	~r_ChunkDataCache();

	void SetChunkQuadCount( int longitude, int latitude, unsigned short cout );
	unsigned short SetChunkQuadCount( int longitude, int latitude ) const;

	void GetChunkWaterHexCount( int longitude, int latitude, unsigned short cout );
	unsigned short SetChunkWaterHexCount( int longitude, int latitude ) const;

	private:

	void SaveChunkMatrix( const char* file_name, const unsigned short* matrix ) const;
	void LoadChunkMatrix( const char* file_name, unsigned short* matrix );

	unsigned short chunk_quad_count_matrix_[ CHUNK_MATRIX_WIDTH_ * CHUNK_MATRIX_HEIGHT_ ];
	unsigned short chunk_water_hex_count_matrix_[ CHUNK_MATRIX_WIDTH_ * CHUNK_MATRIX_HEIGHT_];
};



inline void r_ChunkDataCache::SetChunkQuadCount( int longitude, int latitude, unsigned short count )
{
	chunk_quad_count_matrix_[ longitude + latitude * CHUNK_MATRIX_WIDTH_ ]= count;
}
inline unsigned short r_ChunkDataCache::SetChunkQuadCount( int longitude, int latitude ) const
{
	return chunk_quad_count_matrix_[ longitude + latitude * CHUNK_MATRIX_WIDTH_ ];
}

inline void r_ChunkDataCache::GetChunkWaterHexCount( int longitude, int latitude, unsigned short count )
{
	chunk_water_hex_count_matrix_[ longitude + latitude * CHUNK_MATRIX_WIDTH_ ]= count;
}
inline unsigned short r_ChunkDataCache::SetChunkWaterHexCount( int longitude, int latitude ) const
{
	return chunk_water_hex_count_matrix_[ longitude + latitude * CHUNK_MATRIX_WIDTH_ ];
}

#endif//CHUNK_DATA_CACHE_HPP
