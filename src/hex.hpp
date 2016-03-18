#pragma once
#include <cstdint>

#define H_CHUNK_WIDTH_LOG2 4
#define H_CHUNK_WIDTH (1 << H_CHUNK_WIDTH_LOG2)

#define H_CHUNK_HEIGHT_LOG2 7
#define H_CHUNK_HEIGHT (1 << H_CHUNK_HEIGHT_LOG2)

#define H_MIN_CHUNKS_LOG2 3
#define H_MIN_CHUNKS (1 << H_MIN_CHUNKS_LOG2)
#define H_MAX_CHUNKS_LOG2 6
#define H_MAX_CHUNKS (1 << H_MAX_CHUNKS_LOG2)

#define H_MIN_CHUNKS_IN_CLUSTER 2
#define H_MAX_CHUNKS_IN_CLUSTER 6

#define H_MIN_LONGITUDE (-512)
#define H_MAX_LONGITUDE ( 511)
#define H_MIN_LATITUDE  (-512)
#define H_MAX_LATITUDE  ( 511)


#define H_WORLD_REGION_SIZE_X 24
#define H_WORLD_REGION_SIZE_Y 20


#define H_MAX_WATER_LEVEL_LOG2 14
#define H_MAX_WATER_LEVEL (1 << H_MAX_WATER_LEVEL_LOG2)

#define H_CHUNK_INITIAL_WATER_BLOCK_COUNT 256u

#define H_HEXAGON_EDGE_SIZE 0.5773502691f
#define H_HEXAGON_INNER_RADIUS 0.5f

//space scale vector
#define H_SPACE_SCALE_VECTOR_X 0.8660254037f

#define H_MAX_SUN_LIGHT 8
#define H_MAX_FIRE_LIGHT 13

// Put new block types at the end of list.
enum class h_BlockType : unsigned short
{
	Air= 0,//MUST BE 0
	SphericalBlock= 1, // MUST BE 1
	Stone,
	Soil,
	Wood,
	Grass,
	Water,
	Sand,
	Foliage,
	FireStone,
	Brick,

	FailingBlock,

	NumBlockTypes,
	Unknown= 65535
};

/* COORDINATE SYSTEM:
  __
 /  \
 \__/

 y
 ^
 |
 |
 |z( up )
 +-------> x

    f
    ______
fl /      \ fr
  /   up   \
  \        /
bl \______/ br
      b

  __    __
 /03\__/23\
 \__/13\__/33\
 /02\__/22\__/
 \__/12\__/32\
 /01\__/21\__/
 \__/11\__/31\
 /00\__/20\__/
 \__/10\__/30\
    \__/  \__/

*/

enum class h_Direction : unsigned char
{
	Forward= 0, //y+1
	Back,       //y-1

	ForwardRight= 2, //x+1 y+?
	BackLeft,        //x-1 y-?

	ForwardLeft= 4,  //x-1 y+?
	BackRight,       //x+1 y-?

	Up= 6, //z+1
	Down,  //z-1

	Unknown= 255
};

enum h_WorldMoveDirection:
unsigned char
{
	NORTH,
	SOUTH,
	EAST,
	WEST
};

// Transparancy for blocks rendering.
enum h_VisibleTransparency :
uint8_t
{
	TRANSPARENCY_SOLID=     0, //rock, sand, wood, other blocks with non-alpha textures
	TRANSPARENCY_GLASS=     1, //glass and other syntetic transparent materials
	TRANSPARENCY_GREENERY = 1, //greenery, leafs, grass
	TRANSPARENCY_GAS =      1, //visibly gas ( smoke, plasma, etc )
	TRANSPARENCY_LIQUID=    3, //water, oil, other liquids
	TRANSPARENCY_AIR =      3, //air transparency
};

typedef uint8_t h_CombinedTransparency;
#define H_VISIBLY_TRANSPARENCY_BITS            0b00000011
#define H_DIRECT_SUN_LIGHT_TRANSPARENCY_BIT    0b00000100
#define H_SECONDARY_SUN_LIGHT_TRANSPARENCY_BIT 0b00001000
#define H_FIRE_LIGHT_TRANSPARENCY_BIT          0b00010000

