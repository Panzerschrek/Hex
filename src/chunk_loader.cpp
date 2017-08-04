#include "chunk_loader.hpp"
#include "world_loading.hpp"
#include "console.hpp"

#include "math_lib/assert.hpp"
#include "math_lib/math.hpp"

static int GetChunkIndexInRegion( int longitude, int latitude )
{
	int rel_lon= m_Math::ModNonNegativeRemainder( longitude, H_WORLD_REGION_SIZE_X );
	int rel_lat= m_Math::ModNonNegativeRemainder( latitude , H_WORLD_REGION_SIZE_Y );
	return rel_lon + rel_lat * H_WORLD_REGION_SIZE_X;
}

//struct for loaded region data. chunks are compressed
struct h_ChunkLoader::RegionData final
{
	const int longitude;
	const int latitude ;

	unsigned int chunks_used;
	bool chunks_used_flags[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	//data of each chunk in region ( each chunk stored separatly ). Data is compressed
	h_BinaryStorage chunk_data[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	RegionData( int in_longitude, int in_latitude )
		: longitude(in_longitude)
		, latitude (in_latitude )
		, chunks_used(0)
	{
		for( bool& flag : chunks_used_flags )
			flag= false;
	}
};

h_ChunkLoader::h_ChunkLoader( QString world_directory )
	: regions_path_(world_directory)
{}

h_ChunkLoader::~h_ChunkLoader()
{
	H_ASSERT( regions_.empty() && "All chunks must be released before ChunkLoader destruction" );
}

h_BinaryStorage& h_ChunkLoader::GetChunkData( int longitude, int latitude )
{
	RegionData& reg= GetRegionForCoordinates( longitude, latitude );
	int ind= GetChunkIndexInRegion( longitude, latitude );

	if( !reg.chunks_used_flags[ind] )
	{
		reg.chunks_used_flags[ind]= true;
		reg.chunks_used++;
	}
	return reg.chunk_data[ind];
}

void h_ChunkLoader::FreeChunkData( int longitude, int latitude )
{
	RegionData& reg= GetRegionForCoordinates( longitude, latitude );
	int ind= GetChunkIndexInRegion( longitude, latitude );

	if( reg.chunks_used_flags[ind] )
	{
		reg.chunks_used_flags[ind]= false;
		reg.chunks_used--;
	}

	if( reg.chunks_used == 0 )
	{
		//free region here
		for( RegionDataPtr& region : regions_ )
		{
			if( region.get() == &reg )
			{
				SaveRegion( reg );
				if( region != regions_.back() )
					region= std::move( regions_.back() );
				regions_.resize( regions_.size() - 1 );
				break;
			}
		}
	}
}

h_ChunkLoader::RegionData& h_ChunkLoader::GetRegionForCoordinates( int longitude, int latitude )
{
	int region_longitude= m_Math::DivNonNegativeRemainder( longitude, H_WORLD_REGION_SIZE_X ) * H_WORLD_REGION_SIZE_X;
	int region_latitude = m_Math::DivNonNegativeRemainder( latitude , H_WORLD_REGION_SIZE_Y ) * H_WORLD_REGION_SIZE_Y;

	for( RegionDataPtr& region : regions_ )
	{
		if(
			region->longitude == region_longitude &&
			region->latitude  == region_latitude )
			return *region;
	}

	//here load new region from disk
	RegionDataPtr new_region( new RegionData( region_longitude, region_latitude ) );
	LoadRegion( *new_region );
	regions_.push_back( std::move(new_region) );

	return *regions_.back();
}

void h_ChunkLoader::GetRegionFileName( QString& out_name, int reg_longitude, int reg_latitude ) const
{
	out_name= regions_path_ + "/lon_";
	out_name+= QString::number( reg_longitude ) + "_lat_";
	out_name+= QString::number( reg_latitude  ) + "_.region";
	//for example: "world/lon_42_lat_-34_.region"
}

void h_ChunkLoader::LoadRegion( RegionData& region )
{
	//todo: add reading of file with unicode name
	QString file_name;
	GetRegionFileName( file_name, region.longitude, region.latitude );
	FILE* f= fopen( file_name.toLocal8Bit().constData(), "rb" );
	if( f == nullptr )
	{
		//make new region, with no chunks
		h_Console::Info( "Region file for coordinates ", region.longitude, " ", region.latitude,  " not found." );
		return;
	}

	HEXREGION_header header;
	fread( &header, 1, sizeof(HEXREGION_header), f );

	H_ASSERT( region.longitude == header.longitude );
	H_ASSERT( region.latitude  == header.latitude  );

	for( unsigned int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
	{
		int chunk_data_size= header.chunk_lumps[i].size;
		if( chunk_data_size == 0 )
			continue;

		region.chunk_data[i].resize( chunk_data_size );
		fread( region.chunk_data[i].data(), 1, chunk_data_size, f );
	}

	fclose(f);
}

void h_ChunkLoader::SaveRegion( const RegionData& region ) const
{
	//todo: add reading of file with unicode name
	QString file_name;
	GetRegionFileName( file_name, region.longitude, region.latitude );
	FILE* f= fopen( file_name.toLocal8Bit().constData(), "wb" );
	if( f == nullptr )
	{
		h_Console::Error(
			"Can not open file \"", file_name.toLocal8Bit().constData(), "\" for region saving." );
		return;
	}

	HEXREGION_header header;
	header.longitude= region.longitude;
	header.latitude = region.latitude ;

	//write to header size of compressed chunks
	for( unsigned int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
		header.chunk_lumps[i].size= region.chunk_data[i].size();

	fwrite( &header, 1, sizeof(HEXREGION_header), f );

	for( unsigned int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
	{
		int chunk_data_size= header.chunk_lumps[i].size;
		if( chunk_data_size == 0 )
			continue;

		fwrite( region.chunk_data[i].data(), 1, chunk_data_size, f );
	}

	fclose(f);
}

void h_ChunkLoader::ForceSaveAllChunks() const
{
	for( const RegionDataPtr& region : regions_ )
		SaveRegion( *region );
}
