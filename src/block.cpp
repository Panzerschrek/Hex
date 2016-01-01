#include <cctype>
#include <cstring>

#include "hex.hpp"
#include "block.hpp"

h_TransparencyType h_Block::Transparency() const
{
	h_TransparencyType t;
	switch(type_)
	{
	case AIR:
		t= TRANSPARENCY_AIR;
		break;

	case FOLIAGE:
		t= TRANSPARENCY_GREENERY;
		break;

	case WATER:
		t= TRANSPARENCY_LIQUID;
		break;

	case FIRE_STONE:
		t= TRANSPARENCY_SOLID;
		break;

	default:
		t= TRANSPARENCY_SOLID;
		break;
	};
	return t;
}

#define MACRO_TO_STR(X) #X

static const char* const block_names[NUM_BLOCK_TYPES]=
{
	MACRO_TO_STR(AIR),
	MACRO_TO_STR(SPHERICAL_BLOCK),
	MACRO_TO_STR(STONE),
	MACRO_TO_STR(SOIL),
	MACRO_TO_STR(WOOD),
	MACRO_TO_STR(GRASS),
	MACRO_TO_STR(WATER),
	MACRO_TO_STR(SAND),
	MACRO_TO_STR(FOLIAGE),
	MACRO_TO_STR(FIRE_STONE),
	MACRO_TO_STR(BRICK),
};

static const char* const block_type_unknown= "BLOCK_UNKNOWN";

h_BlockType h_Block::GetGetBlockTypeByName( const char* name )
{
	if( name == nullptr )
		return BLOCK_UNKNOWN;

	char buff[128];
	int i= 0;
	while( name[i] != 0 )
	{
		buff[i]= char( toupper( name[i] ) );
		i++;
	}
	buff[i]= 0;

	for( i= 0; i< NUM_BLOCK_TYPES; i++ )
	{
		if( !strcmp( buff, block_names[i] ) )
			return h_BlockType( i );
	}

	return BLOCK_UNKNOWN;
}

const char* h_Block::GetBlockName( h_BlockType type )
{
	if( type < NUM_BLOCK_TYPES )
		return block_names[ type ];

	return block_type_unknown;
}

static const char* direction_names[]= {
	"FORWARD",
	"BACK",
	"FORWARD_RIGHT",
	"BACK_LEFT",
	"FORWARD_LEFT",
	"BACK_RIGHT",
	"UP",
	"DOWN"
};

h_Direction h_Block::GetDirectionByName( const char* name )
{
	if( name == nullptr )
		return DIRECTION_UNKNOWN;

	char buff[128];
	int i= 0;
	while( name[i] != 0 )
	{
		buff[i]= char( toupper( name[i] ) );
		i++;
	}
	buff[i]= 0;

	for( i= 0; i< 8; i++ )
	{
		if( !strcmp( buff, direction_names[i] ) )
			return h_Direction(i);
	}
	return DIRECTION_UNKNOWN;
}
