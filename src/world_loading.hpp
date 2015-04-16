#pragma once
#include "hex.hpp"


#define H_CHUNK_FORMAT_VERSION 1
#define H_REGION_FORMAT_VERSION 1
#define H_WORLD_FORMAT_VERSION 1

#define H_CHUNK_FORMAT_HEADER	"HEXchunk"
#define H_REGION_FORMAT_HEADER	"HEXregio"
#define H_WORLD_FORMAT_HEADER	"HEXworld"

struct FileLump
{
	int offset;//offset from effective file data ( discards header )
	int size;

	//if struct changed, this must be changed too
	void Read( QDataStream& stream );
	void Write( QDataStream& stream );

};

struct HEXCHUNK_header
{
	signed char format_key[8];
	int version;
	int datalen;

	short longitude;
	short latitude;

	unsigned short water_block_count;

	FileLump blocks_data;

	//if struct changed, this must be changed too
	void Read( QDataStream& stream );
	void Write( QDataStream& stream );
};


struct HEXREGION_header
{
	signed char format_key[8];
	int version;
	int datalen;

	//cordinates of region beginning. each chunk in region has coordinates, greather ot equal than this
	short longitude;
	short latitude;

	//if lump.size is zero, chunk does not exist
	FileLump chunk_lumps[ H_WORLD_REGION_SIZE_X * H_WORLD_REGION_SIZE_Y ];

	//if struct changed, this must be changed too
	void Read( QDataStream& stream );
	void Write( QDataStream& stream );
};

struct WORLD_header
{
	signed char format_key[8];
	int version;
	int datalen;

	//if struct changed, this must be changed too
	void Read( QDataStream& stream );
	void Write( QDataStream& stream );
};

