#ifndef BLOCK_CPP
#define BLOCK_CPP

#include "hex.hpp"
#include "block.hpp"



h_TransparencyType h_Block::Transparency()
{
	h_TransparencyType t;
	switch (type)
	{
		case AIR:
		t= TRANSPARENCY_AIR;
		break;

		case WATER:
		t= TRANSPARENCY_LIQUID;
		break;

		case FIRE:
		t= TRANSPARENCY_SOLID;
		break;

		default:
		t= TRANSPARENCY_SOLID;
		break;
	};
	return t;
   /* if( type == AIR )
        return TRANSPARENCY_AIR;
    else if( type == WATER )
        return TRANSPARENCY_LIQUID;
    else
        return TRANSPARENCY_SOLID;*/
}

#define MACRO_TO_STR(X) #X

static const char* block_names[NUM_BLOCK_TYPES]= {
    MACRO_TO_STR(AIR),
    MACRO_TO_STR(SPHERICAL_BLOCK),
    MACRO_TO_STR(STONE),
    MACRO_TO_STR(SOIL),
    MACRO_TO_STR(WOOD),
    MACRO_TO_STR(GRASS),
    MACRO_TO_STR(WATER),
    MACRO_TO_STR(SAND),
    MACRO_TO_STR(FOLIAGE),
    MACRO_TO_STR(FIRE) };

h_BlockType h_Block::GetGetBlockTypeByName( const char* name )
{
    if( name == NULL )
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

static const char* direction_names[]={
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
    if( name == NULL )
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

#endif//BLOCK_CPP
