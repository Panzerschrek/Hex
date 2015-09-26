#include "chunk_loader.hpp"

#include "math_lib/m_math.h"

h_RegionData::h_RegionData()
{
	for( unsigned int i= 0; i< sizeof(chunks_used_flags) / sizeof(bool); i++ )
		chunks_used_flags[i]= false;
	chunks_used= 0;
}

h_ChunkLoader::h_ChunkLoader( QString world_directory ):
	regions_path(world_directory)
{
}

h_ChunkLoader::~h_ChunkLoader()
{
for( auto region : regions )
	{
		delete region;
	}
}
QByteArray& h_ChunkLoader::GetChunkData( int longitude, int latitude )
{
	h_RegionData* reg= GetRegionForCoordinates( longitude, latitude );

	int rel_lon= m_Math::ModNonNegativeRemainder( longitude, H_WORLD_REGION_SIZE_X ),
				 rel_lat= m_Math::ModNonNegativeRemainder( latitude, H_WORLD_REGION_SIZE_Y );
	int ind= rel_lon + rel_lat * H_WORLD_REGION_SIZE_X;

	if( !reg->chunks_used_flags[ind] )
	{
		reg->chunks_used++;
		reg->chunks_used_flags[ind]= true;
	}
	return reg->chunk_data[ind];
}
void h_ChunkLoader::FreeChunkData( int longitude, int latitude )
{
	h_RegionData* reg= GetRegionForCoordinates( longitude, latitude );

	int rel_lon= m_Math::ModNonNegativeRemainder( longitude, H_WORLD_REGION_SIZE_X ),
				 rel_lat= m_Math::ModNonNegativeRemainder( latitude, H_WORLD_REGION_SIZE_Y );
	int ind= rel_lon + rel_lat * H_WORLD_REGION_SIZE_X;

	if( reg->chunks_used_flags[ind] )
	{
		reg->chunks_used_flags[ind]= false;
		reg->chunks_used--;
	}

	if( reg->chunks_used == 0 )
	{
		//free region here
		for( unsigned int i= 0; i< regions.size(); i++ )
		{
			if( regions[i] == reg )
			{
				SaveRegion( reg );
				delete reg;
				if( i != regions.size()-1 )
					regions[i]= regions[ regions.size()-1 ];
				regions.resize( regions.size()-1 );
				break;
			}
		}
	}
}

h_RegionData* h_ChunkLoader::GetRegionForCoordinates( int longitude, int latitude )
{
	int region_longitude= m_Math::DivNonNegativeRemainder( longitude, H_WORLD_REGION_SIZE_X ) * H_WORLD_REGION_SIZE_X;
	int region_latitude= m_Math::DivNonNegativeRemainder( latitude, H_WORLD_REGION_SIZE_Y ) * H_WORLD_REGION_SIZE_Y;

for( auto region : regions )
	{
		if(
			region->header.longitude == region_longitude &&
			region->header.latitude == region_latitude )
			return region;
	}

	//here load new region from disk
	h_RegionData* new_region= new h_RegionData();
	LoadRegion( region_longitude, region_latitude, new_region );
	regions.push_back( new_region );
	return new_region;
}

void h_ChunkLoader::GetRegionFileName( QString& out_name, int reg_longitude, int reg_latitude )
{
	out_name= regions_path + "/lon_";
	out_name+= QString::number( reg_longitude ) + "_lat_";
	out_name+= QString::number( reg_latitude ) + "_.region";
	//for example: "world/lon_42_lat_-34_.region"

}

void h_ChunkLoader::LoadRegion( int longitude, int latitude, h_RegionData* region )
{
	//todo: add reading of file with unicode name
	QString file_name;
	GetRegionFileName( file_name, longitude, latitude );
	FILE* f= fopen( file_name.toLocal8Bit().constData(), "rb" );
	if( f == nullptr )
	{
		//make new region, with no chunks
		region->header.longitude= longitude;
		region->header.latitude= latitude;
		for( int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
			region->chunk_data[i].resize(0);
		return;
	}

	fread( &region->header, 1, sizeof(HEXREGION_header), f );

	for( int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
	{
		int chunk_data_size= region->header.chunk_lumps[i].size;
		if( chunk_data_size == 0 )
		{
			region->chunk_data[i].resize(0);
			continue;
		}

		char* chunk_data= new char[ chunk_data_size ];
		fread( chunk_data, 1, chunk_data_size, f );
		region->chunk_data[i].setRawData( chunk_data, chunk_data_size );
	}

	fclose(f);
}


void h_ChunkLoader::SaveRegion( h_RegionData* region )
{
	//todo: add reading of file with unicode name
	QString file_name;
	GetRegionFileName( file_name, region->header.longitude, region->header.latitude );
	FILE* f= fopen( file_name.toLocal8Bit().constData(), "wb" );
	if( f == nullptr )
	{
		//fail here
	}

	//write to header size of compressed chunks
	for( int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
		region->header.chunk_lumps[i].size= region->chunk_data[i].size();


	fwrite( &region->header, 1, sizeof(HEXREGION_header), f );

	for( int i= 0; i< H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y; i++ )
	{
		int chunk_data_size= region->header.chunk_lumps[i].size;
		if( chunk_data_size == 0 )
			continue;

		fwrite( region->chunk_data[i].constData(), 1, chunk_data_size, f );
	}

	fclose(f);
}


void h_ChunkLoader::ForceSaveAllChunks()
{
	for( unsigned int i= 0; i< regions.size(); i++ )
		SaveRegion( regions[i] );
}

