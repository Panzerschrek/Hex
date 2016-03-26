#include "chunk.hpp"
#include "world.hpp"
#include "math_lib/assert.hpp"
#include "math_lib/rand.hpp"
#include "world_generator/world_generator.hpp"

inline unsigned char BlockAddrToX( unsigned int block_addr )
{
	H_ASSERT( block_addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	return block_addr >> (H_CHUNK_WIDTH_LOG2 + H_CHUNK_HEIGHT_LOG2);
}

inline unsigned char BlockAddrToY( unsigned int block_addr )
{
	H_ASSERT( block_addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	return (block_addr >> H_CHUNK_HEIGHT_LOG2) & (H_CHUNK_WIDTH-1);
}

inline unsigned char BlockAddrToZ( unsigned int block_addr )
{
	H_ASSERT( block_addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	return block_addr & (H_CHUNK_HEIGHT-1);
}

// Check point in chunk relative coordinates.
inline static bool InChunkBorders( int x, int y )
{
	return
		x >= 0 && x < H_CHUNK_WIDTH &&
		y >= 0 && y < H_CHUNK_WIDTH;
}

h_Chunk::h_Chunk( h_World* world, int longitude, int latitude, const g_WorldGenerator* generator )
	: world_(world)
	, longitude_(longitude), latitude_(latitude)
	, need_update_light_(false)
{
	GenChunk( generator );
	PlantGrass();
	PlantTrees( generator );
	GenWaterBlocks();
	ActivateGrass();
	MakeLight();
}

h_Chunk::h_Chunk( h_World* world, const HEXCHUNK_header& header, QDataStream& stream )
	: world_(world)
	, longitude_(header.longitude)
	, latitude_ (header.latitude )
	, need_update_light_(false)
{
	GenChunkFromFile( stream );
	MakeLight();
}

h_Chunk::~h_Chunk()
{
}

bool h_Chunk::IsEdgeChunk() const
{
	return
		longitude_ == world_->Longitude() || latitude_ == world_->Latitude() ||
		longitude_ == ( world_->Longitude() + int(world_->ChunkNumberX()) - 1 ) ||
		latitude_  == ( world_->Latitude () + int(world_->ChunkNumberY()) - 1 );
}

void h_Chunk::SaveBlock( QDataStream& stream, const h_Block* block )
{
	stream << ((unsigned short)block->Type());

	switch(block->Type())
	{
	case h_BlockType::Air:
	case h_BlockType::SphericalBlock:
	case h_BlockType::Stone:
	case h_BlockType::Soil:
	case h_BlockType::Wood:
	case h_BlockType::Sand:
	case h_BlockType::Foliage:
	case h_BlockType::FireStone:
	case h_BlockType::Brick:
		break;

	case h_BlockType::Water:
		stream << static_cast< const h_LiquidBlock*>(block)->LiquidLevel();
		break;

	case h_BlockType::FailingBlock:
		{
			const h_FailingBlock* failing_block= static_cast<const h_FailingBlock*>(block);
			SaveBlock( stream, failing_block->GetBlock() );

			// TODO - save other failing block data
		}
		break;

	case h_BlockType::Grass:
		stream << static_cast< const h_GrassBlock* >(block)->IsActive();
		break;

	case h_BlockType::BrickPlate:
		{
			auto nonstandard_form_block= static_cast< const h_NonstandardFormBlock* >(block);
			stream << ( (unsigned char) nonstandard_form_block->Direction() );
		}
		break;

	case h_BlockType::NumBlockTypes:
	case h_BlockType::Unknown:
		break;
	};
}

h_Block* h_Chunk::LoadBlock( QDataStream& stream, unsigned int block_addr )
{
	unsigned short s_block_id;
	//HACK. if block type basic integer type changed, this must be changed too
	stream >> s_block_id;
	h_BlockType block_id= (h_BlockType)s_block_id;

	h_Block* block;

	switch(block_id)
	{
	case h_BlockType::Air:
	case h_BlockType::SphericalBlock:
	case h_BlockType::Stone:
	case h_BlockType::Soil:
	case h_BlockType::Wood:
	case h_BlockType::Sand:
	case h_BlockType::Foliage:
	case h_BlockType::Brick:
		block= world_->NormalBlock(block_id);
		break;

	case h_BlockType::FireStone:
		{
			h_LightSource* light_source= new h_LightSource( h_BlockType::FireStone, H_MAX_FIRE_LIGHT );
			light_source->x_= BlockAddrToX(block_addr);
			light_source->y_= BlockAddrToY(block_addr);
			light_source->z_= BlockAddrToZ(block_addr);

			light_source_list_.push_back( light_source );

			block= light_source;
		}
		break;

	case h_BlockType::Water:
		{
			unsigned short water_level;
			stream >> water_level;

			h_LiquidBlock* liquid_block= NewWaterBlock();

			liquid_block->x_= BlockAddrToX(block_addr);
			liquid_block->y_= BlockAddrToY(block_addr);
			liquid_block->z_= BlockAddrToZ(block_addr);

			liquid_block->SetLiquidLevel( water_level );

			block= liquid_block;
		}
		break;

	case h_BlockType::FailingBlock:
		{
			h_FailingBlock* failing_block=
				failing_blocks_alocatior_.New(
					LoadBlock( stream, block_addr ),
					BlockAddrToX(block_addr),
					BlockAddrToY(block_addr),
					BlockAddrToZ(block_addr) );

			failing_blocks_.push_back( failing_block );

			block= failing_block;
		}
		break;

	case h_BlockType::Grass:
		{
			h_GrassBlock* grass_block;
			bool is_active;

			stream >> is_active;

			if( is_active )
			{
				grass_block=
					NewActiveGrassBlock(
						BlockAddrToX(block_addr),
						BlockAddrToY(block_addr),
						BlockAddrToZ(block_addr) );
			}
			else
				grass_block= world_->UnactiveGrassBlock();

			block= grass_block;
		}
		break;

	case h_BlockType::BrickPlate:
		{
			unsigned char direction;
			stream >> direction;

			block= NewNonstandardFormBlock(
					BlockAddrToX(block_addr),
					BlockAddrToY(block_addr),
					BlockAddrToZ(block_addr),
					h_BlockType::BrickPlate, static_cast<h_Direction>(direction) );
		}
		break;

	default:
		H_ASSERT(false);
		block= world_->NormalBlock(h_BlockType::Air);
		break;
	};

	return block;
}

void h_Chunk::GenChunk( const g_WorldGenerator* generator )
{
	short x, y, z;
	short h, soil_h;

	unsigned char sea_level= generator->GetSeaLevel();

	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	{
		for( y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			h=
				generator->GetGroundLevel(
					(longitude_ << H_CHUNK_WIDTH_LOG2) + x,
					(latitude_  << H_CHUNK_WIDTH_LOG2) + y );
			//if( longitude == -1 &&  latitude == -1 )h= 3;

			soil_h= 4;

			// TODO - optimize
			SetBlock( x, y, 0, world_->NormalBlock( h_BlockType::SphericalBlock) );
			SetBlock( x, y, H_CHUNK_HEIGHT-1, world_->NormalBlock( h_BlockType::SphericalBlock ) );
			for( z= 1; z< h - soil_h; z++ )
				SetBlock( x, y, z, world_->NormalBlock( h_BlockType::Stone ) );

			for( ; z< h; z++ )
				SetBlock( x, y, z, world_->NormalBlock( h_BlockType::Soil ) );

			//if( !( longitude == -1 && latitude == -1 ) )
			for( ; z<= sea_level; z++ )
				SetBlock( x, y, z, world_->NormalBlock( h_BlockType::Water ) );

			for( ; z< H_CHUNK_HEIGHT-1; z++ )
				SetBlock( x, y, z, world_->NormalBlock( h_BlockType::Air ) );
		}//for y
	}//for x
}

void h_Chunk::GenChunkFromFile( QDataStream& stream )
{
	for( int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
	{
		h_Block* block= LoadBlock( stream, i );
		SetBlock( i, block );
	}
}

void h_Chunk::SaveChunkToFile( QDataStream& stream )
{
	for( int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
		SaveBlock( stream, blocks_[i] );
}

void h_Chunk::PlantTrees( const g_WorldGenerator* generator )
{
	generator->PlantTreesForChunk(
		longitude_, latitude_,
		[this, generator]( int x, int y )
		{
			const int c_tree_planting_max_altitude= H_CHUNK_HEIGHT - 20;

			int tree_x= x - (longitude_ << H_CHUNK_WIDTH_LOG2);
			int tree_y= y - (latitude_  << H_CHUNK_WIDTH_LOG2);

			int tree_z= generator->GetGroundLevel( x, y );
			if( tree_z > 2 + (int)generator->GetSeaLevel() && tree_z <= c_tree_planting_max_altitude )
			{
				if( (tree_x ^ tree_y ^ tree_z) & 2 )
					PlantBigTree( tree_x, tree_y, tree_z );
				else
					PlantTree( tree_x, tree_y, tree_z );
			}
		});
}

void h_Chunk::PlantTree( short x, short y, short z )
{
	int h;
	for( h= z; h< z + 4; h++ )
	{
		if( InChunkBorders( x, y ) )
		{
			SetBlock( x, y, h, world_->NormalBlock( h_BlockType::Wood ) );
		}

		if( h - z > 1 )
		{
			if( InChunkBorders( x, y + 1 ) )
				SetBlock( x, y + 1, h, world_->NormalBlock( h_BlockType::Foliage ) );

			if( InChunkBorders( x, y - 1 ) )
				SetBlock( x, y - 1, h, world_->NormalBlock( h_BlockType::Foliage ) );

			if( InChunkBorders( x + 1, y + ((x+1)&1) ) )
				SetBlock( x + 1, y + ((x+1)&1), h, world_->NormalBlock( h_BlockType::Foliage ) );
			if( InChunkBorders( x + 1, y - (x&1) ) )
				SetBlock( x + 1, y - (x&1), h, world_->NormalBlock( h_BlockType::Foliage ) );

			if( InChunkBorders( x - 1, y + ((x+1)&1) ) )
				SetBlock( x - 1, y + ((x+1)&1), h, world_->NormalBlock( h_BlockType::Foliage ) );
			if( InChunkBorders( x - 1, y - (x&1) ) )
				SetBlock( x - 1, y - (x&1), h, world_->NormalBlock( h_BlockType::Foliage ) );
		}
	}
	if( InChunkBorders( x, y ) )
		SetBlock( x, y, h, world_->NormalBlock( h_BlockType::Foliage ) );
}

void h_Chunk::PlantBigTree( short x, short y, short z )
{
	for( short zz= z; zz< z+7; zz++ )
	{
		if( InChunkBorders( x, y ) )
			SetBlock( x, y, zz, world_->NormalBlock( h_BlockType::Wood ) );
		if( InChunkBorders( x+1, y+((x+1)&1) ) )
			SetBlock( x+1, y+((x+1)&1), zz, world_->NormalBlock( h_BlockType::Wood ) );
		if( InChunkBorders( x+1, y-(x&1) ) )
			SetBlock( x+1, y-(x&1), zz, world_->NormalBlock( h_BlockType::Wood ) );
	}
	for( short zz= z+7; zz< z+9; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y ) )
			SetBlock( x, y, zz, b );
		if( InChunkBorders( x+1, y+((x+1)&1) ) )
			SetBlock( x+1, y+((x+1)&1), zz, b );
		if( InChunkBorders( x+1, y-(x&1) ) )
			SetBlock( x+1, y-(x&1), zz, b );
	}


	for( short zz= z+2; zz< z+8; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y+1 ) )
			SetBlock( x, y+1, zz, b );
		if( InChunkBorders( x, y-1 ) )
			SetBlock( x, y-1, zz, b );
		if( InChunkBorders( x+2, y ) )
			SetBlock( x+2, y, zz, b );
		if( InChunkBorders( x+2, y+1 ) )
			SetBlock( x+2, y+1, zz, b );
		if( InChunkBorders( x+2, y-1 ) )
			SetBlock( x+2, y-1, zz, b );
		if( InChunkBorders( x-1, y-(x&1) ) )
			SetBlock( x-1, y-(x&1), zz, b );
		if( InChunkBorders( x-1, y+((x+1)&1) ) )
			SetBlock( x-1, y+((x+1)&1), zz, b );
		if( InChunkBorders( x+1, y+1+((x+1)&1) ) )
			SetBlock( x+1, y+1+((x+1)&1), zz, b );
		if( InChunkBorders( x+1, y-1-(x&1) ) )
			SetBlock( x+1, y-1-(x&1), zz, b );
	}
	for( short zz= z+3; zz< z+7; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y+2 ) )
			SetBlock( x, y+2, zz, b );
		if( InChunkBorders( x, y-2 ) )
			SetBlock( x, y-2, zz, b );

		if( InChunkBorders( x-1, y-1-(x&1) ) )
			SetBlock( x-1, y-1-(x&1), zz, b );
		if( InChunkBorders( x-1, y+1+((x+1)&1) ) )
			SetBlock( x-1, y+1+((x+1)&1), zz, b );

		if( InChunkBorders( x+3, y+((x+1)&1) ) )
			SetBlock( x+3, y+((x+1)&1), zz, b );
		if( InChunkBorders( x+3, y-(x&1) ) )
			SetBlock( x+3, y-(x&1), zz, b );
	}

	for( short zz= z+4; zz< z+6; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x-2, y ) )
			SetBlock( x-2, y, zz, b );
		if( InChunkBorders( x-2, y-1 ) )
			SetBlock( x-2, y-1, zz, b );
		if( InChunkBorders( x-2, y+1 ) )
			SetBlock( x-2, y+1, zz, b );

		if( InChunkBorders( x+1, y+2+((x+1)&1) ) )
			SetBlock( x+1, y+2+((x+1)&1), zz, b );
		if( InChunkBorders( x+1,y-2-(x&1) ) )
			SetBlock( x+1,y-2-(x&1), zz, b );

		if( InChunkBorders( x+2, y+2 ) )
			SetBlock( x+2, y+2, zz, b );
		if( InChunkBorders( x+2, y-2 ) )
			SetBlock( x+2, y-2, zz, b );

		if( InChunkBorders( x+3, y+1+((x+1)&1) ) )
			SetBlock( x+3, y+1+((x+1)&1), zz, b );
		if( InChunkBorders( x+3, y-1-(x&1) ) )
			SetBlock( x+3, y-1-(x&1), zz, b );
	}
}

void h_Chunk::PlantGrass()
{
	for( int x= 0; x< H_CHUNK_WIDTH; x++ )
	for( int y= 0; y< H_CHUNK_WIDTH; y++ )
	{
		int addr= BlockAddr( x, y, 0 );

		for( int z= H_CHUNK_HEIGHT - 2; z > 0; z-- )
			if ( blocks_[ addr + z ]->Type() != h_BlockType::Air )
			{
				if( blocks_[ addr + z ]->Type() == h_BlockType::Soil )
					SetBlock( addr + z, world_->UnactiveGrassBlock() );

				break;
			}
	}
}

void h_Chunk::ActivateGrass()
{
	for( int x= 0; x< H_CHUNK_WIDTH; x++ )
	for( int y= 0; y< H_CHUNK_WIDTH; y++ )
	{
		int addr= BlockAddr( x, y, 0 );

		for( int z= H_CHUNK_HEIGHT - 2; z > 0; z-- )
			if ( blocks_[ addr + z ]->Type() == h_BlockType::Grass )
				blocks_[ addr + z ]= NewActiveGrassBlock( x, y, z );
	}
}

unsigned int h_Chunk::CalculateWaterBlockCount()
{
	unsigned int c= 0;
	for( unsigned int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
		if( blocks_[i]->Type() == h_BlockType::Water )
			c++;
	return c;
}

void h_Chunk::GenWaterBlocks()
{
	short x, y, z, addr = 0;
	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	for( y= 0; y< H_CHUNK_WIDTH; y++ )
	for( z= 0; z< H_CHUNK_HEIGHT; z++, addr++ )
	{
		if( blocks_[ addr ]->Type() == h_BlockType::Water )
		{
			h_LiquidBlock* block= water_blocks_allocator_.New();
			block->x_= x;
			block->y_= y;
			block->z_= z;
			blocks_[ addr ]= block;
			block->SetLiquidLevel( H_MAX_WATER_LEVEL );
			water_block_list_.push_back( block );
		}
	}
}

void h_Chunk::MakeLight()
{
	for( unsigned int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( unsigned int y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			unsigned int addr= BlockAddr( x, y, H_CHUNK_HEIGHT-2 );
			unsigned int z;

			for( z= H_CHUNK_HEIGHT-2; z> 0; z--, addr-- )
			{
				if( !( transparency_[ addr ] & H_DIRECT_SUN_LIGHT_TRANSPARENCY_BIT ) )
					break;
				sun_light_map_[ addr ]= H_MAX_SUN_LIGHT;
				fire_light_map_[ addr ]= 0;
			}
			for( ; z > 0; z--, addr-- )
			{
				sun_light_map_[ addr ]= 0;
				fire_light_map_[ addr ]= 0;
			}
		}
}

void h_Chunk::SunRelight()
{
	for( unsigned int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( unsigned int y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			unsigned int addr= BlockAddr( x, y, H_CHUNK_HEIGHT-2 );
			unsigned int z;

			for( z= H_CHUNK_HEIGHT-2; z> 0; z--, addr-- )
			{
				if( !( transparency_[ addr ] & H_DIRECT_SUN_LIGHT_TRANSPARENCY_BIT ) )
					break;
				sun_light_map_[ addr ]= H_MAX_SUN_LIGHT;
			}
			for( ; z > 0; z--, addr-- )
				sun_light_map_[ addr ]= 0;
		}
}

h_LiquidBlock* h_Chunk::NewWaterBlock()
{
	h_LiquidBlock* b= water_blocks_allocator_.New();
	water_block_list_.push_back( b );
	return b;
}

void h_Chunk::DeleteWaterBlock( h_LiquidBlock* b )
{
	// Do not remove block from list here.
	//This must did outer code.

	water_blocks_allocator_.Delete(b);
}

h_LightSource* h_Chunk::NewLightSource( short x, short y, short z, h_BlockType type )
{
	h_LightSource* s= new h_LightSource( type );
	light_source_list_.push_back(s);
	s->x_= x;
	s->y_= y;
	s->z_= z;
	return s;
}

void h_Chunk::DeleteLightSource( short x, short y, short z )
{
	h_LightSource* s= static_cast<h_LightSource*>( GetBlock( x, y, z ) );

	for( size_t i= 0; i < light_source_list_.size(); i++ )
	{
		if( light_source_list_[i] == s )
		{
			delete s;
			if( i < light_source_list_.size() - 1 )
				light_source_list_[i]= light_source_list_.back();

			light_source_list_.pop_back();
			break;
		}
	}
}

h_NonstandardFormBlock* h_Chunk::NewNonstandardFormBlock(
		unsigned char x, unsigned char y, unsigned char z,
		h_BlockType type, h_Direction direction )
{
	h_NonstandardFormBlock* block=
		nonstandard_form_blocks_allocator_.New( x, y, z, type, direction );

	nonstandard_form_blocks_.push_back(block);

	return block;
}

h_GrassBlock* h_Chunk::NewActiveGrassBlock( unsigned char x, unsigned char y, unsigned char z )
{
	h_GrassBlock* grass_block=
		active_grass_blocks_allocator_.New(
			x, y, z, true );

	active_grass_blocks_.push_back( grass_block );

	return grass_block;
}

void h_Chunk::ProcessFailingBlocks()
{
	for( unsigned int i= 0; i < failing_blocks_.size(); )
	{
		h_FailingBlock* b= failing_blocks_[i];
		i++;

		unsigned char old_z= b->GetZ() >> 16;
		b->Tick();
		unsigned char new_z= b->GetZ() >> 16;

		if( old_z != new_z )
		{
			unsigned int block_addr= BlockAddr( b->GetX(), b->GetY(), 0 );
			int global_x= b->GetX() + ( (longitude_ - world_->Longitude()) << H_CHUNK_WIDTH_LOG2 );
			int global_y= b->GetY() + ( (latitude_  - world_->Latitude ()) << H_CHUNK_WIDTH_LOG2 );

			h_Block* lower_block= blocks_[ block_addr + old_z - 1 ];
			if( lower_block->Type() != h_BlockType::Air )
			{
				// failing blocks can fall to water.
				if( lower_block->Type() == h_BlockType::Water )
				{
					h_LiquidBlock* water_block= static_cast<h_LiquidBlock*>(lower_block);
					water_block->z_= old_z;

					SetBlock( block_addr + old_z , water_block );
					SetBlock( block_addr + new_z, b );

					world_->UpdateWaterInRadius( global_x, global_y, new_z );
				}
				else
				{
					SetBlock( block_addr + old_z, b->GetBlock() );

					world_->UpdateInRadius(
						global_x, global_y,
						world_->RelightBlockAdd( global_x, global_y, old_z ) );

					i--;
					if( i + 1 != failing_blocks_.size() )
						failing_blocks_[i]= failing_blocks_.back();
					failing_blocks_.pop_back();
					failing_blocks_alocatior_.Delete( b );
				}
			}
			else
			{
				SetBlock( block_addr + old_z, world_->NormalBlock( h_BlockType::Air ) );
				SetBlock( block_addr + new_z, b );
			}

			// Check block, upper for this, which can start fail, or do something else.
			world_->CheckBlockNeighbors( global_x, global_y, old_z );
		}
	}
}

unsigned int h_Chunk::GetWaterColumnHeight( short x, short y, short z )
{
	unsigned int h= (z-1) * H_MAX_WATER_LEVEL;
	unsigned int addr= BlockAddr( x, y, z );
	while(  blocks_[addr]->Type() == h_BlockType::Water )
	{
		unsigned int level= ((h_LiquidBlock*)blocks_[addr])->LiquidLevel();
		if( level > H_MAX_WATER_LEVEL )
			level= H_MAX_WATER_LEVEL;
		h+= level;
		addr++;
	}
	return h;
}
