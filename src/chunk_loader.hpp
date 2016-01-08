#pragma once
#include <memory>
#include <vector>

#include <QByteArray>
#include <QString>

class h_ChunkLoader final
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
	struct RegionData;
	typedef std::unique_ptr<RegionData> RegionDataPtr;

private:
	//find loaded region or loads it
	RegionData& GetRegionForCoordinates( int longitude, int latitude );
	void LoadRegion( RegionData& region );
	void SaveRegion( const RegionData& region ) const;

	void GetRegionFileName( QString& out_name, int reg_longitude, int reg_latitude ) const;

private:
	std::vector< RegionDataPtr > regions_;
	QString regions_path_;
};
