#include <algorithm>
#include <cctype>
#include <cstring>

#include "hex.hpp"
#include "block.hpp"

struct BlockProperties
{
	h_VisibleTransparency default_transparency;
	h_BlockForm form;

	bool transparent_for_fire_light : 1;
	bool transparent_for_direct_sun_light : 1;
	bool transparent_for_secondary_sun_light : 1;
	bool is_failing : 1;
	bool is_technical : 1;
};

static_assert( sizeof(BlockProperties) == 3, "Unexpected size" );

static constexpr h_CombinedTransparency GetCombinedTransparency( const BlockProperties& properties )
{
	return
		properties.default_transparency |
		( h_CombinedTransparency(properties.transparent_for_direct_sun_light   ) << 2 ) |
		( h_CombinedTransparency(properties.transparent_for_secondary_sun_light) << 3 ) |
		( h_CombinedTransparency(properties.transparent_for_fire_light         ) << 4 );
}

// Properties of all common blocks.
// Preprocesor used for compile-time initialization, because it fails on structures copying.
#define g_trivial_blocks_properties \
{\
	.default_transparency= TRANSPARENCY_SOLID,\
	.form= h_BlockForm::Full,\
	.transparent_for_fire_light= false,\
	.transparent_for_direct_sun_light= false,\
	.transparent_for_secondary_sun_light= false,\
	.is_failing= false,\
	.is_technical= false,\
}

static const constexpr BlockProperties g_blocks_properties[ size_t(h_BlockType::NumBlockTypes) ]=
{
	[size_t(h_BlockType::Air)]=
	{
		.default_transparency= TRANSPARENCY_AIR,
		.form= h_BlockForm::Full,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= true,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false,
		.is_technical= true,
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
		.form= h_BlockForm::Full,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false,
		.is_technical= false,
	},

	[size_t(h_BlockType::Sand)]=
	{
		.default_transparency= TRANSPARENCY_SOLID,
		.form= h_BlockForm::Full,
		.transparent_for_fire_light= false,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= false,
		.is_failing= true,
		.is_technical= false,
	},

	[size_t(h_BlockType::Foliage)]=
	{
		.default_transparency= TRANSPARENCY_GREENERY,
		.form= h_BlockForm::Full,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= true,
		.is_failing= true,
		.is_technical= false,
	},

	[size_t(h_BlockType::FireStone)]=
	{
		.default_transparency= TRANSPARENCY_SOLID,
		.form= h_BlockForm::Full,
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
		.form= h_BlockForm::Full,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= true,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false, // this property has no sense for failing block
		.is_technical= true,
	},

	[size_t(h_BlockType::BrickPlate)]=
	{
		.default_transparency= TRANSPARENCY_AIR,
		.form= h_BlockForm::Plate,
		.transparent_for_fire_light= true,
		.transparent_for_direct_sun_light= false,
		.transparent_for_secondary_sun_light= true,
		.is_failing= false,
		.is_technical= false,
	},
};

static const constexpr h_CombinedTransparency g_blocks_combined_transparency[ size_t(h_BlockType::NumBlockTypes) ]=
{

#define BLOCK_PROCESS_FUNC(x) [size_t(h_BlockType::x)]= GetCombinedTransparency( g_blocks_properties[size_t(h_BlockType::x)] )
#include "blocks_list.hpp"
#undef BLOCK_PROCESS_FUNC

};

static const char* const g_block_names[ size_t(h_BlockType::NumBlockTypes) ]=
{

#define BLOCK_PROCESS_FUNC(x) #x
#include "blocks_list.hpp"
#undef BLOCK_PROCESS_FUNC

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

bool h_Block::IsTechnicalType( h_BlockType type )
{
	return g_blocks_properties[ size_t(type) ].is_technical;
}

h_BlockForm h_Block::Form( h_BlockType type )
{
	return g_blocks_properties[ size_t(type) ].form;
}

/*
----------------h_Block--------------
*/

h_Block::h_Block( h_BlockType type, unsigned short additional_data )
	: type_(type)
	, additional_data_(additional_data)
	, combined_transparency_( g_blocks_combined_transparency[ size_t(type) ] )
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
---------------h_NonstandardFormBlock-----------
*/

h_NonstandardFormBlock::h_NonstandardFormBlock(
	unsigned char x, unsigned char y, unsigned char z,
	h_BlockType type, h_Direction direction )
	: h_Block(type)
	, x_(x), y_(y), z_(z)
	, direction_(direction)
{
	H_ASSERT( Form(type) != h_BlockForm::Full );
	H_ASSERT( x < H_CHUNK_WIDTH );
	H_ASSERT( y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 1 && z < H_CHUNK_HEIGHT - 1 );

	H_ASSERT
	(
		( Form(type) == h_BlockForm::Plate &&
		(direction == h_Direction::Up || direction == h_Direction::Down) )
		||
		( Form(type) == h_BlockForm::Bisected &&
		direction >= h_Direction::Forward && direction <= h_Direction::BackRight )
	);
}

h_Direction h_NonstandardFormBlock::Direction() const
{
	return direction_;
}

unsigned char h_NonstandardFormBlock::GetX() const
{
	return x_;
}

unsigned char h_NonstandardFormBlock::GetY() const
{
	return y_;
}

unsigned char h_NonstandardFormBlock::GetZ() const
{
	return z_;
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
	static const constexpr fixed16_t c_acceleration= 10 << 8;
	static const constexpr unsigned int c_tick_of_max_speed= 25;
	static const constexpr fixed16_t c_max_speed= c_tick_of_max_speed * c_acceleration;
	static_assert( c_max_speed < (1 << 16), "Blocks must fail at speed, less than 1 block per tick." );

	failig_start_ticks_++;

	// S= a * t^2 / 2
	fixed16_t dz;
	if( failig_start_ticks_ <= c_tick_of_max_speed )
		dz= ( c_acceleration * (failig_start_ticks_ * failig_start_ticks_) ) >> 1;
	else
		dz=
			( ( c_acceleration * (c_tick_of_max_speed * c_tick_of_max_speed) ) >> 1 ) +
			c_max_speed * ( failig_start_ticks_ - c_tick_of_max_speed );

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
