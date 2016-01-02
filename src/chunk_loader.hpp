#pragma once
#include <memory>
#include <vector>

#include "world_loading.hpp"

//struct for loaded region data. chunks are compressed
struct h_RegionData
{
	HEXREGION_header header;
	//data of each chunk in region ( each chunk stored separatly ). Data is compressed
	QByteArray chunk_data[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	bool chunks_used_flags[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];
	int chunks_used;

	h_RegionData();
	~h_RegionData() {}
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
	void ForceSaveAllChunks();

private:
	//find loaded region or loads it
	h_RegionData& GetRegionForCoordinates( int longitude, int latitude );
	void LoadRegion( int longitude, int latitude, h_RegionData& region );
	void SaveRegion( h_RegionData& region );

	void GetRegionFileName( QString& out_name, int reg_longitude, int reg_latitude );

private:
	std::vector< h_RegionDataPtr > regions_;
	QString regions_path_;

};
