#include "chunk.hpp"
#include "world.hpp"
#include "math_lib/assert.hpp"
#include "math_lib/rand.hpp"
#include "world_generator/world_generator.hpp"

// Check point in chunk relative coordinates.
inline static bool InChunkBorders( int x, int y )
{
	return
		x >= 0 && x < H_CHUNK_WIDTH &&
		y >= 0 && y < H_CHUNK_WIDTH;
}

const std::vector<h_FailingBlock*>& h_Chunk::GetFailingBlocks() const
{
	return failing_blocks_;
}

bool h_Chunk::IsEdgeChunk() const
{
	return
		longitude_ == world_->Longitude() || latitude_ == world_->Latitude() ||
		longitude_ == ( world_->Longitude() + int(world_->ChunkNumberX()) - 1 ) ||
		latitude_  == ( world_->Latitude () + int(world_->ChunkNumberY()) - 1 );
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
			SetBlockAndTransparency( x, y, 0, world_->NormalBlock( h_BlockType::SphericalBlock), TRANSPARENCY_SOLID );
			SetBlockAndTransparency( x, y, H_CHUNK_HEIGHT-1, world_->NormalBlock( h_BlockType::SphericalBlock ), TRANSPARENCY_SOLID );
			for( z= 1; z< h - soil_h; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( h_BlockType::Stone ), TRANSPARENCY_SOLID );

			for( ; z< h; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( h_BlockType::Soil ), TRANSPARENCY_SOLID );

			//if( !( longitude == -1 && latitude == -1 ) )
			for( ; z<= sea_level; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( h_BlockType::Water ), TRANSPARENCY_LIQUID );

			for( ; z< H_CHUNK_HEIGHT-1; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( h_BlockType::Air ), TRANSPARENCY_AIR );
		}//for y
	}//for x
}

void h_Chunk::GenChunkFromFile( QDataStream& stream )
{
	h_LiquidBlock* liquid_block;

	for( int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
	{
		h_BlockType block_id;
		unsigned short s_block_id;
		//HACK. if block type basic integer type changed, this must be changed too
		stream >> s_block_id;
		block_id= (h_BlockType)s_block_id;

		unsigned short water_level;
		h_LightSource* light_source;
		h_Block* b;

		switch(block_id)
		{
		case h_BlockType::Air:
		case h_BlockType::SphericalBlock:
		case h_BlockType::Stone:
		case h_BlockType::Soil:
		case h_BlockType::Wood:
		case h_BlockType::Grass:
		case h_BlockType::Sand:
		case h_BlockType::Foliage:
		case h_BlockType::Brick:
			blocks_[i]= b= world_->NormalBlock(block_id);
			transparency_[i]= b->Transparency();
			break;

		case h_BlockType::FireStone:
			blocks_[i]= light_source= new h_LightSource( h_BlockType::FireStone, H_MAX_FIRE_LIGHT );
			light_source_list_.Add( light_source );
			transparency_[i]= blocks_[i]->Transparency();

			light_source->x_= i >> (H_CHUNK_WIDTH_LOG2 + H_CHUNK_HEIGHT_LOG2);
			light_source->y_= (i>>H_CHUNK_HEIGHT_LOG2) & (H_CHUNK_WIDTH-1);
			light_source->z_= i & (H_CHUNK_HEIGHT-1);
			break;

		case h_BlockType::Water:
			liquid_block= water_blocks_allocator_.New();
			blocks_[i]= liquid_block;
			transparency_[i]= TRANSPARENCY_LIQUID;
			stream >> water_level;

			liquid_block->x_= i >> (H_CHUNK_WIDTH_LOG2 + H_CHUNK_HEIGHT_LOG2);
			liquid_block->y_= (i>>H_CHUNK_HEIGHT_LOG2) & (H_CHUNK_WIDTH-1);
			liquid_block->z_= i & (H_CHUNK_HEIGHT-1);

			liquid_block->SetLiquidLevel( water_level );
			water_blocks_data.water_block_list.Add( liquid_block );
			break;

		default:
			H_ASSERT(false);
			break;
		};
	}//for blocks in
}

void h_Chunk::SaveChunkToFile( QDataStream& stream )
{
	for( int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
	{
		h_Block* b= blocks_[i];

		stream << ((unsigned short)b->Type());

		switch(b->Type())
		{
		case h_BlockType::Air:
		case h_BlockType::SphericalBlock:
		case h_BlockType::Stone:
		case h_BlockType::Soil:
		case h_BlockType::Wood:
		case h_BlockType::Grass:
		case h_BlockType::Sand:
		case h_BlockType::Foliage:
		case h_BlockType::FireStone:
		case h_BlockType::Brick:
			break;

		case h_BlockType::Water:
			stream << ((h_LiquidBlock*)b)->LiquidLevel();
			break;

		case h_BlockType::NumBlockTypes:
		case h_BlockType::Unknown:
			break;
		};
	}

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
			SetTransparency( x, y, h, TRANSPARENCY_SOLID );
			SetBlock( x, y, h, world_->NormalBlock( h_BlockType::Wood ) );
		}

		if( h - z > 1 )
		{
			if( InChunkBorders( x, y + 1 ) )
				SetBlockAndTransparency( x, y + 1, h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );

			if( InChunkBorders( x, y - 1 ) )
				SetBlockAndTransparency( x, y - 1, h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );

			if( InChunkBorders( x + 1, y + ((x+1)&1) ) )
				SetBlockAndTransparency( x + 1, y + ((x+1)&1), h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );
			if( InChunkBorders( x + 1, y - (x&1) ) )
				SetBlockAndTransparency( x + 1, y - (x&1), h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );

			if( InChunkBorders( x - 1, y + ((x+1)&1) ) )
				SetBlockAndTransparency( x - 1, y + ((x+1)&1), h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );
			if( InChunkBorders( x - 1, y - (x&1) ) )
				SetBlockAndTransparency( x - 1, y - (x&1), h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );
		}
	}
	if( InChunkBorders( x, y ) )
		SetBlockAndTransparency( x, y, h, world_->NormalBlock( h_BlockType::Foliage ), TRANSPARENCY_GREENERY );
}

void h_Chunk::PlantBigTree( short x, short y, short z )
{
	for( short zz= z; zz< z+7; zz++ )
	{
		if( InChunkBorders( x, y ) )
			SetBlockAndTransparency( x, y, zz, world_->NormalBlock( h_BlockType::Wood ),  TRANSPARENCY_SOLID );
		if( InChunkBorders( x+1, y+((x+1)&1) ) )
			SetBlockAndTransparency( x+1, y+((x+1)&1), zz, world_->NormalBlock( h_BlockType::Wood ),  TRANSPARENCY_SOLID );
		if( InChunkBorders( x+1, y-(x&1) ) )
			SetBlockAndTransparency( x+1, y-(x&1), zz, world_->NormalBlock( h_BlockType::Wood ),  TRANSPARENCY_SOLID );
	}
	for( short zz= z+7; zz< z+9; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y ) )
			SetBlockAndTransparency( x, y, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+1, y+((x+1)&1) ) )
			SetBlockAndTransparency( x+1, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+1, y-(x&1) ) )
			SetBlockAndTransparency( x+1, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}


	for( short zz= z+2; zz< z+8; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y+1 ) )
			SetBlockAndTransparency( x, y+1, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x, y-1 ) )
			SetBlockAndTransparency( x, y-1, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+2, y ) )
			SetBlockAndTransparency( x+2, y, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+2, y+1 ) )
			SetBlockAndTransparency( x+2, y+1, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+2, y-1 ) )
			SetBlockAndTransparency( x+2, y-1, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x-1, y-(x&1) ) )
			SetBlockAndTransparency( x-1, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x-1, y+((x+1)&1) ) )
			SetBlockAndTransparency( x-1, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+1, y+1+((x+1)&1) ) )
			SetBlockAndTransparency( x+1, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+1, y-1-(x&1) ) )
			SetBlockAndTransparency( x+1, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}
	for( short zz= z+3; zz< z+7; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x, y+2 ) )
			SetBlockAndTransparency( x, y+2, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x, y-2 ) )
			SetBlockAndTransparency( x, y-2, zz, b,  TRANSPARENCY_GREENERY );

		if( InChunkBorders( x-1, y-1-(x&1) ) )
			SetBlockAndTransparency( x-1, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x-1, y+1+((x+1)&1) ) )
			SetBlockAndTransparency( x-1, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );

		if( InChunkBorders( x+3, y+((x+1)&1) ) )
			SetBlockAndTransparency( x+3, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+3, y-(x&1) ) )
			SetBlockAndTransparency( x+3, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}

	for( short zz= z+4; zz< z+6; zz++ )
	{
		h_Block* b= world_->NormalBlock( h_BlockType::Foliage );
		if( InChunkBorders( x-2, y ) )
			SetBlockAndTransparency( x-2, y, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x-2, y-1 ) )
			SetBlockAndTransparency( x-2, y-1, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x-2, y+1 ) )
			SetBlockAndTransparency( x-2, y+1, zz, b,  TRANSPARENCY_GREENERY );

		if( InChunkBorders( x+1, y+2+((x+1)&1) ) )
			SetBlockAndTransparency( x+1, y+2+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+1,y-2-(x&1) ) )
			SetBlockAndTransparency( x+1,y-2-(x&1), zz, b,  TRANSPARENCY_GREENERY );

		if( InChunkBorders( x+2, y+2 ) )
			SetBlockAndTransparency( x+2, y+2, zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+2, y-2 ) )
			SetBlockAndTransparency( x+2, y-2, zz, b,  TRANSPARENCY_GREENERY );

		if( InChunkBorders( x+3, y+1+((x+1)&1) ) )
			SetBlockAndTransparency( x+3, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		if( InChunkBorders( x+3, y-1-(x&1) ) )
			SetBlockAndTransparency( x+3, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}
}


void h_Chunk::PlantGrass()
{
	//TODO - optomize
	short x, y, z;
	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	{
		for( y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			for( z= H_CHUNK_HEIGHT - 2; z> 0; z-- )
				if ( GetBlock( x, y, z )->Type() != h_BlockType::Air )
					break;

			if( GetBlock( x, y, z )->Type() == h_BlockType::Soil )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( h_BlockType::Grass ), TRANSPARENCY_SOLID );
		}
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
			water_blocks_data.water_block_list.Add( block );
		}
	}
}

h_LiquidBlock* h_Chunk::NewWaterBlock()
{
	h_LiquidBlock* b= water_blocks_allocator_.New();
	water_blocks_data.water_block_list.Add( b );
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
	h_LightSource* s;
	light_source_list_.Add( s= new h_LightSource( type ) );
	s->x_= x;
	s->y_= y;
	s->z_= z;
	return s;
}
void h_Chunk::DeleteLightSource( short x, short y, short z )
{
	h_LightSource* s= (h_LightSource*) GetBlock( x, y, z );

	m_Collection< h_LightSource* >::Iterator it( &light_source_list_ );
	for( it.Begin(); it.IsValid() ; it.Next() )
		if( *it == s )
		{
			it.RemoveCurrent();
			delete s;
			break;
		}
}

void h_Chunk::ProcessFailingBlocks()
{
	for( unsigned int i= 0; i < failing_blocks_.size(); )
	{
		h_FailingBlock* b= failing_blocks_[i];

		unsigned char old_z= b->GetZ() >> 16;
		b->Tick();
		unsigned char new_z= b->GetZ() >> 16;

		if( old_z != new_z )
		{
			unsigned int block_addr= BlockAddr( b->GetX(), b->GetY(), 0 );
			int global_x= b->GetX() + ( (longitude_ - world_->Longitude()) << H_CHUNK_WIDTH_LOG2 );
			int global_y= b->GetY() + ( (latitude_  - world_->Latitude ()) << H_CHUNK_WIDTH_LOG2 );

			if( blocks_[ block_addr + old_z - 1 ]->Type() != h_BlockType::Air )
			{
				blocks_[ block_addr + old_z ]= b->GetBlock();
				transparency_[ block_addr + old_z ]= b->GetBlock()->Transparency();

				world_->UpdateInRadius(
					global_x, global_y,
					world_->RelightBlockAdd( global_x, global_y, old_z ) );

				if( i != failing_blocks_.size() - 1 )
					failing_blocks_[i]= failing_blocks_.back();
				failing_blocks_.pop_back();
				failing_blocks_alocatior_.Delete( b );

				continue;
			}
			else
			{
				blocks_[ block_addr + old_z ]= world_->NormalBlock( h_BlockType::Air );
				blocks_[ block_addr + new_z ]= b;
			}

			// Check block, upper for this, which can start fail.
			world_->CheckFailingBlock( global_x, global_y, old_z + 1 );
		}
		i++;
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
				if( blocks_[ addr ]->Type() != h_BlockType::Air )
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
				if( blocks_[ addr ]->Type() != h_BlockType::Air )
					break;
				sun_light_map_[ addr ]= H_MAX_SUN_LIGHT;
			}
			for( ; z > 0; z--, addr-- )
				sun_light_map_[ addr ]= 0;
		}
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

void h_Chunk::ReCalculateHeightmap()
{
	//TODO - Does this need?
	/*
	for( short y= 0; y< H_CHUNK_WIDTH; y++ )
		for( short x= 0; x< H_CHUNK_WIDTH; y++ )
		{
			int addr= (H_CHUNK_HEIGHT-2) | ( y << H_CHUNK_HEIGHT_LOG2 ) | ( x << ( H_CHUNK_HEIGHT_LOG2 + H_CHUNK_WIDTH_LOG2 ) );
			for( short z= H_CHUNK_HEIGHT-2; z>=0; z--, addr-- )
			{
				if( blocks[addr]->Type() != AIR )
				{
					height_map_[ x | (y<<H_CHUNK_WIDTH_LOG2) ]= z;
					break;
				}
			}
		}
		*/
}
