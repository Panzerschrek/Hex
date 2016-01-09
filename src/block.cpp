#include <cctype>
#include <cstring>

#include "hex.hpp"
#include "block.hpp"

h_TransparencyType h_Block::Transparency() const
{
	switch(type_)
	{
	case h_BlockType::Air:
		return TRANSPARENCY_AIR;

	case h_BlockType::Foliage:
		return TRANSPARENCY_GREENERY;

	case h_BlockType::Water:
		return TRANSPARENCY_LIQUID;

	case h_BlockType::FireStone:
		return TRANSPARENCY_SOLID;

	default:
		return TRANSPARENCY_SOLID;
		break;
	};
}

// TODO - make better way to convert enum to string. Preprocessor, for example.
#define MACRO_TO_STR(X) #X

static const char* const block_names[ size_t(h_BlockType::NumBlockTypes) ]=
{
	MACRO_TO_STR(Air),
	MACRO_TO_STR(SphericalBlock),
	MACRO_TO_STR(Stone),
	MACRO_TO_STR(Soil),
	MACRO_TO_STR(Wood),
	MACRO_TO_STR(Grass),
	MACRO_TO_STR(Water),
	MACRO_TO_STR(Sand),
	MACRO_TO_STR(Foliage),
	MACRO_TO_STR(FireStone),
	MACRO_TO_STR(Brick),
};

static const char* const block_type_unknown= "Unknown";

h_BlockType h_Block::GetGetBlockTypeByName( const char* name )
{
	if( name == nullptr ) return h_BlockType::Unknown;

	for( unsigned int i= 0; i< (unsigned int)h_BlockType::NumBlockTypes; i++ )
	{
		const char* s= name;
		const char* d= block_names[i];
		while( *s && *d && toupper(*s) == toupper(*d) )
		{
			s++;
			d++;
		}
		if( *s == '\0' && *d == '\0' ) return static_cast<h_BlockType>(i);
	}

	return h_BlockType::Unknown;
}

const char* h_Block::GetBlockName( h_BlockType type )
{
	if( type < h_BlockType::NumBlockTypes )
		return block_names[ static_cast<size_t>(type) ];

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
		return h_Direction::Unknown;

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
	return h_Direction::Unknown;
}
