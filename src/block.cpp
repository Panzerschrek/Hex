#include <algorithm>
#include <cctype>
#include <cstring>

#include "hex.hpp"
#include "block.hpp"


static constexpr h_CombinedTransparency GetCombinedTransparency(
	const h_VisibleTransparency visible_transparency,
	bool direct_sun_light_transparency,
	bool secondary_sun_light_transparency,
	bool fire_light_transaprency )
{
	return
		visible_transparency |
		( h_CombinedTransparency(direct_sun_light_transparency   ) << 2 ) |
		( h_CombinedTransparency(secondary_sun_light_transparency) << 3 ) |
		( h_CombinedTransparency(fire_light_transaprency         ) << 4 );
}

// Structure for block properties, used in heavy-loaded algorithms (lighting, water flow, grass reproducing, etc. ).
// Make this size of this struct so small, as you can.
struct BlockProperties
{
	h_VisibleTransparency default_transparency;

	bool transparent_for_fire_light : 1;
	bool transparent_for_direct_sun_light : 1;
	bool transparent_for_secondary_sun_light : 1;
	bool is_failing : 1;
	bool is_technical : 1;
};

static_assert( sizeof(BlockProperties) == 2, "Unexpected size" );

// Properties of all common blocks.
// Preprocesor used for compile-time initialization, because it fails on structures copying.
#define g_trivial_blocks_properties \
{\
	.default_transparency= TRANSPARENCY_SOLID,\
	.transparent_for_fire_light= false,\
	.transparent_for_direct_sun_light= false,\
	.transparent_for_secondary_sun_light= false,\
	.is_failing= false,\
	.is_technical= false,\
}

// We need this table in ReadOnly section of result eecutable file.
static const BlockProperties g_blocks_properties[ size_t(h_BlockType::NumBlockTypes) ]=
{
	[size_t(h_BlockType::Air)]=
	{
		.default_transparency= TRANSPARENCY_AIR,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= true,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false,
		.is_technical= false,
	},

	[size_t(h_BlockType::SphericalBlock)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::Stone)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::Soil)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::Wood)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::Grass)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::Water)]=
	{
		.default_transparency= TRANSPARENCY_LIQUID,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false,
		.is_technical= false,
	},

	[size_t(h_BlockType::Sand)]=
	{
		.default_transparency= TRANSPARENCY_SOLID,
		.transparent_for_fire_light= false,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= false,
		.is_failing= true,
		.is_technical= false,
	},

	[size_t(h_BlockType::Foliage)]=
	{
		.default_transparency= TRANSPARENCY_GREENERY,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= true,
		.is_failing= true,
		.is_technical= false,
	},

	[size_t(h_BlockType::FireStone)]=
	{
		.default_transparency= TRANSPARENCY_SOLID,
		.transparent_for_fire_light= false,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= false,
		.is_failing= false,
		.is_technical= false,
	},

	[size_t(h_BlockType::Brick)]=
	g_trivial_blocks_properties,

	[size_t(h_BlockType::FailingBlock)]=
	{
		.default_transparency= TRANSPARENCY_AIR,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= true,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false, // this property has no sense for failing block
		.is_technical= true,
	},
};

// TODO - make better way to convert enum to string. Preprocessor, for example.
#define MACRO_TO_STR(X) #X

static const char* const g_block_names[ size_t(h_BlockType::NumBlockTypes) ]=
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
	MACRO_TO_STR(FailingBlock),
};

static const char* const g_block_type_unknown= "Unknown";

h_BlockType h_Block::GetGetBlockTypeByName( const char* name )
{
	if( name == nullptr ) return h_BlockType::Unknown;

	for( unsigned int i= 0; i< (unsigned int)h_BlockType::NumBlockTypes; i++ )
	{
		const char* s= name;
		const char* d=  g_block_names[i];
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
		return  g_block_names[ static_cast<size_t>(type) ];

	return g_block_type_unknown;
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

/*
----------------h_Block--------------
*/

h_Block::h_Block( h_BlockType type, unsigned short additional_data )
	: type_(type)
	, additional_data_(additional_data)
	, combined_transparency_(
		GetCombinedTransparency(
			g_blocks_properties[ size_t(type_) ].default_transparency,
			g_blocks_properties[ size_t(type_) ].transparent_for_direct_sun_light,
			g_blocks_properties[ size_t(type_) ].transparent_for_secondary_sun_light,
			g_blocks_properties[ size_t(type_) ].transparent_for_fire_light ) )
{}

h_BlockType h_Block::Type() const
{
	return type_;
}

h_CombinedTransparency h_Block::CombinedTransparency() const
{
	return combined_transparency_;
}

unsigned short h_Block::AdditionalData() const
{
	return additional_data_;
}

/*
----------------h_LiquidBlock--------------
*/

h_LiquidBlock::h_LiquidBlock( h_BlockType type, unsigned short liquid_level )
	: h_Block( type, liquid_level )
{}

h_LiquidBlock::h_LiquidBlock()
	: h_Block( h_BlockType::Water, H_MAX_WATER_LEVEL )
{}

unsigned short h_LiquidBlock::LiquidLevel() const
{
	return additional_data_;
}

void h_LiquidBlock::SetLiquidLevel( unsigned short l )
{
	additional_data_= l;
}

void h_LiquidBlock::IncreaseLiquidLevel( unsigned short l )
{
	additional_data_+= l;
}

void h_LiquidBlock::DecreaseLiquidLevel( unsigned short l )
{
	additional_data_-= l;
}

/*
----------------h_LightSource--------------
*/

h_LightSource::h_LightSource( h_BlockType type, unsigned char light_level )
	: h_Block( type, light_level )
{}

unsigned char h_LightSource::LightLevel() const
{
	return h_Block::additional_data_;
}

void h_LightSource::SetLightLevel( unsigned char level )
{
	h_Block::additional_data_= level;
}

/*
----------------h_FailingBlock--------------
*/

h_FailingBlock::h_FailingBlock(
	h_Block* block,
	unsigned char x, unsigned char y, unsigned char z )
	: h_Block(h_BlockType::FailingBlock)
	, block_(block)
	, x_(x), y_(y)
	, failing_start_z_(z)
	, failig_start_ticks_(0)
	, z_( z << 16 )
{
	H_ASSERT( block->Type() != h_BlockType::FailingBlock );
	H_ASSERT( x < H_CHUNK_WIDTH );
	H_ASSERT( y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 1 && z < H_CHUNK_HEIGHT - 1 );
}

h_Block* h_FailingBlock::GetBlock()
{
	return block_;
}

const h_Block* h_FailingBlock::GetBlock() const
{
	return block_;
}

void h_FailingBlock::Tick()
{
	static const fixed16_t c_acceleration= 3 << 8;
	static const unsigned int c_tick_of_max_speed= 40;

	failig_start_ticks_++;

	// S= a * t^2 / 2
	fixed16_t dz;
	if( failig_start_ticks_ <= c_tick_of_max_speed )
		dz= ( c_acceleration * (failig_start_ticks_ * failig_start_ticks_) ) >> 1;
	else
		dz=
			( ( c_acceleration * (c_tick_of_max_speed * c_tick_of_max_speed) ) >> 1 ) +
			( c_tick_of_max_speed * c_acceleration ) * ( failig_start_ticks_ - c_tick_of_max_speed );

	z_= ( failing_start_z_ << 16 ) - dz;
}

unsigned char h_FailingBlock::GetX() const
{
	return x_;
}

unsigned char h_FailingBlock::GetY() const
{
	return y_;
}

fixed16_t h_FailingBlock::GetZ() const
{
	return z_;
}

/*
----------------h_GrassBlock--------------
*/

h_GrassBlock::h_GrassBlock(
	unsigned char x, unsigned char y, unsigned char z,
	bool active )
	: h_Block( h_BlockType::Grass )
	, x_(x), y_(y), z_(z)
	, active_(active)
{
	H_ASSERT( x < H_CHUNK_WIDTH );
	H_ASSERT( y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 1 && z < H_CHUNK_HEIGHT - 1 );
}

bool h_GrassBlock::IsActive() const
{
	return active_;
}

unsigned char h_GrassBlock::GetX() const
{
	return x_;
}

unsigned char h_GrassBlock::GetY() const
{
	return y_;
}

unsigned char h_GrassBlock::GetZ() const
{
	return z_;
}
