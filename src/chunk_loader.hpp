#pragma once
#include <memory>
#include <vector>

#include "world_loading.hpp"

//struct for loaded region data. chunks are compressed
struct h_RegionData
{
	const int longitude;
	const int latitude ;

	unsigned int chunks_used;
	bool chunks_used_flags[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	//data of each chunk in region ( each chunk stored separatly ). Data is compressed
	QByteArray chunk_data[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	h_RegionData( int in_longitude, int in_latitude );
};

typedef std::unique_ptr<h_RegionData> h_RegionDataPtr;

class h_ChunkLoader
{
public:
	h_ChunkLoader( QString world_directory );
	~h_ChunkLoader();

	//in\out - compressed data
	//returns container for compressed chunk data ( id need, load it from disk )
	QByteArray& GetChunkData( int longitude, int latitude );
	void FreeChunkData( int longitude, int latitude );

	//save all data of all regions, but not delete it
	void ForceSaveAllChunks() const;

private:
	//find loaded region or loads it
	h_RegionData& GetRegionForCoordinates( int longitude, int latitude );
	void LoadRegion( h_RegionData& region );
	void SaveRegion( const h_RegionData& region ) const;

	void GetRegionFileName( QString& out_name, int reg_longitude, int reg_latitude ) const;

private:
	std::vector< h_RegionDataPtr > regions_;
	QString regions_path_;

};
