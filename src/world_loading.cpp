#include "world_loading.hpp"

void FileLump::Read( QDataStream& stream )
{
	stream>> offset;
	stream>> size;
}

void FileLump::Write( QDataStream& stream )
{
	stream<< offset;
	stream<< size;
}

void HEXCHUNK_header::Read( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream>> format_key[i];

	stream>> version;
	stream>> datalen;

	stream>> longitude;
	stream>> latitude;

	stream>> water_block_count;

	blocks_data.Read( stream );
}

void HEXCHUNK_header::Write( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream<< format_key[i];

	stream<< version;
	stream<< datalen;

	stream<< longitude;
	stream<< latitude;

	stream<< water_block_count;

	blocks_data.Write( stream );
}

void HEXREGION_header::Read( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream>> format_key[i];

	stream>> version;
	stream>> datalen;

	stream>> longitude;
	stream>> latitude;

	for( FileLump& lump : chunk_lumps )
		lump.Read( stream );
}

void HEXREGION_header::Write( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream<< format_key[i];

	stream<< version;
	stream<< datalen;

	stream<< longitude;
	stream<< latitude;

	for( FileLump& lump : chunk_lumps )
		lump.Write( stream );
}

void WORLD_header::Read( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream>> format_key[i];

	stream>> version;
	stream>> datalen;
}

void WORLD_header::Write( QDataStream& stream )
{
	for( unsigned int i= 0; i< sizeof(format_key); i++ )
		stream<< format_key[i];

	stream<< version;
	stream<< datalen;
}
