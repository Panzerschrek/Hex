#include "chunk.hpp"
#include "world.hpp"
#include "math_lib/rand.h"

#define H_SEA_LEVEL (H_CHUNK_HEIGHT/2)

typedef int fixed16_t;

fixed16_t Noise2( int x, int y )
{
	int n = x + y * 57;
	n = (n << 13) ^ n;

	return ( ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff )>>14 ) - 65536;
}

fixed16_t InterpolatedNoise( int x, int y )
{
	int X= x>>6, Y= y>>6;

	fixed16_t dx= x&63, dy= y&63;
	fixed16_t dy1= 64-dy;

	fixed16_t noise[]= {
		Noise2(X, Y),
		Noise2(X + 1, Y),
		Noise2(X + 1, Y + 1),
		Noise2(X, Y + 1)
	};

	fixed16_t interp_x[]= {
		noise[3] * dy + noise[0] * dy1,
		noise[2] * dy + noise[1] * dy1
	};

	return ( interp_x[1] * dx + interp_x[0] * (63-dx) )>>12;
}

fixed16_t FinalNoise( int x, int y )
{
	fixed16_t r= InterpolatedNoise(x, y)>>1;
	r+= InterpolatedNoise(x<<1,y<<1)>>2;
	r+= InterpolatedNoise(x<<2,y<<2)>>3;

	return r;

}

bool h_Chunk::IsEdgeChunk() const
{
	return
		longitude_ == world_->Longitude() || latitude_ == world_->Latitude() ||
		longitude_ == ( world_->Longitude() + world_->ChunkNumberX() - 1 ) ||
		latitude_  == ( world_->Latitude () + world_->ChunkNumberY() - 1 );
}

void h_Chunk::GenChunk()
{
	short x, y, z;
	short h, soil_h;
	short addr;


	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	{
		for( y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			h= (H_CHUNK_HEIGHT/2) + (( 2 * 24 * FinalNoise( short( float(x + longitude_ * H_CHUNK_WIDTH) * H_SPACE_SCALE_VECTOR_X  ),
									   y + latitude_ * H_CHUNK_WIDTH ) ) >>16 ) ;
			//if( longitude == -1 &&  latitude == -1 )h= 3;

			soil_h= 4 + (( 2 * FinalNoise( short( float( x + longitude_ * H_CHUNK_WIDTH ) * H_SPACE_SCALE_VECTOR_X ) * 4,
										   ( y + latitude_ * H_CHUNK_WIDTH ) * 4  ) )>>16 );

			// TODO - optimize
			SetBlockAndTransparency( x, y, 0, world_->NormalBlock( SPHERICAL_BLOCK), TRANSPARENCY_SOLID );
			SetBlockAndTransparency( x, y, H_CHUNK_HEIGHT-1, world_->NormalBlock( SPHERICAL_BLOCK), TRANSPARENCY_SOLID );
			for( z= 1; z< h - soil_h; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( STONE ), TRANSPARENCY_SOLID );

			for( ; z< h; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( SOIL ), TRANSPARENCY_SOLID );

			//if( !( longitude == -1 && latitude == -1 ) )
			for( ; z<= H_SEA_LEVEL; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( WATER ), TRANSPARENCY_LIQUID );

			for( ; z< H_CHUNK_HEIGHT-1; z++ )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( AIR ), TRANSPARENCY_AIR );
		}//for y
	}//for x
}

void h_Chunk::GenChunkFromFile( const HEXCHUNK_header* header, QDataStream& stream )
{
	PrepareWaterBlocksCache( header->water_block_count );
	h_LiquidBlock* water_blocks= water_blocks_data.initial_water_blocks;

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
		case AIR:
		case SPHERICAL_BLOCK:
		case STONE:
		case SOIL:
		case WOOD:
		case GRASS:
		case SAND:
		case FOLIAGE:
			blocks_[i]= b= world_->NormalBlock(block_id);
			transparency_[i]= b->Transparency();
			break;

		case FIRE_STONE:
			blocks_[i]= light_source= new h_LightSource( FIRE_STONE, H_MAX_FIRE_LIGHT );
			light_source_list_.Add( light_source );
			transparency_[i]= blocks_[i]->Transparency();

			light_source->x_= i >> (H_CHUNK_WIDTH_LOG2 + H_CHUNK_HEIGHT_LOG2);
			light_source->y_= (i>>H_CHUNK_HEIGHT_LOG2) & (H_CHUNK_WIDTH-1);
			light_source->z_= i & (H_CHUNK_HEIGHT-1);
			break;

		case WATER:
			blocks_[i]= water_blocks;
			transparency_[i]= TRANSPARENCY_LIQUID;
			stream >> water_level;

			water_blocks->x_= i >> (H_CHUNK_WIDTH_LOG2 + H_CHUNK_HEIGHT_LOG2);
			water_blocks->y_= (i>>H_CHUNK_HEIGHT_LOG2) & (H_CHUNK_WIDTH-1);
			water_blocks->z_= i & (H_CHUNK_HEIGHT-1);

			water_blocks->SetLiquidLevel( water_level );
			water_blocks_data.water_block_list.Add( water_blocks );
			water_blocks++;
			break;

		default:
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

		unsigned short water_level;
		switch(b->Type())
		{
		case AIR:
		case SPHERICAL_BLOCK:
		case STONE:
		case SOIL:
		case WOOD:
		case GRASS:
		case SAND:
		case FOLIAGE:
		case FIRE_STONE:
			break;

		case WATER:
			stream  << ( (unsigned short) ( ((h_LiquidBlock*)b)->LiquidLevel() ) );
			break;

		default:
			break;
		};
	}

}

void h_Chunk::PlantTrees()
{
	short tree_x, tree_y, tree_z, h;
	//short addr;

	static m_Rand r(0);
	for( int n= 0; n< 6; n++ )
	{
		// get_coord:
		tree_x= ( H_CHUNK_WIDTH * r.Rand() ) / r.max_rand;
		tree_y= ( H_CHUNK_WIDTH * r.Rand() ) / r.max_rand;
		if( tree_x < 2 || tree_x > H_CHUNK_WIDTH - 2
				|| tree_y < 2 || tree_y > H_CHUNK_WIDTH - 2 )
			continue;

		for( h= H_CHUNK_HEIGHT - 2; h > 0; h-- )
			if( GetBlock( tree_x, tree_y, h )->Type() !=AIR )
				break;

		if( GetBlock( tree_x, tree_y, h )->Type() != SOIL )
		{
			if( GetBlock( tree_x, tree_y, h )->Type() == GRASS )
				h--;
			else
				continue;
		}
		if(  tree_x >= 5 && tree_x <= 9 && tree_y >= 5 && tree_y <= 9 )
		{
			PlaneBigTree( tree_x, tree_y, h+2 );
			continue;
		}
		continue;

		tree_z= ++h;
		tree_z++;

		for( h= tree_z; h< tree_z + 4; h++ )
		{
			SetTransparency( tree_x, tree_y, h, TRANSPARENCY_SOLID );
			SetBlock( tree_x, tree_y, h, world_->NormalBlock( WOOD ) );

			if( h - tree_z > 1  )
			{
				SetBlockAndTransparency( tree_x, tree_y + 1, h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );

				SetBlockAndTransparency( tree_x, tree_y - 1, h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );

				SetBlockAndTransparency( tree_x + 1, tree_y + ((tree_x+1)&1), h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );
				SetBlockAndTransparency( tree_x + 1, tree_y - (tree_x&1), h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );

				SetBlockAndTransparency( tree_x - 1, tree_y + ((tree_x+1)&1), h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );
				SetBlockAndTransparency( tree_x - 1, tree_y - (tree_x&1), h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );
			}
		}
		SetBlockAndTransparency( tree_x, tree_y, h, world_->NormalBlock( FOLIAGE ), TRANSPARENCY_GREENERY );
	}
}


void h_Chunk::PlaneBigTree( short x, short y, short z )//local coordinates
{
	for( short zz= z; zz< z+7; zz++ )
	{
		SetBlockAndTransparency( x, y, zz, world_->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
		SetBlockAndTransparency( x+1, y+((x+1)&1), zz, world_->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
		SetBlockAndTransparency( x+1, y-(x&1), zz, world_->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
	}
	for( short zz= z+7; zz< z+9; zz++ )
	{
		h_Block* b= world_->NormalBlock( FOLIAGE );
		SetBlockAndTransparency( x, y, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+1, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+1, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}


	for( short zz= z+2; zz< z+8; zz++ )
	{
		h_Block* b= world_->NormalBlock( FOLIAGE );
		SetBlockAndTransparency( x, y+1, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x, y-1, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+2, y, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+2, y+1, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+2, y-1, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x-1, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x-1, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+1, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+1, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}
	for( short zz= z+3; zz< z+7; zz++ )
	{
		h_Block* b= world_->NormalBlock( FOLIAGE );
		SetBlockAndTransparency( x, y+2, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x, y-2, zz, b,  TRANSPARENCY_GREENERY );

		SetBlockAndTransparency( x-1, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x-1, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );

		SetBlockAndTransparency( x+3, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+3, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}

	for( short zz= z+4; zz< z+6; zz++ )
	{
		h_Block* b= world_->NormalBlock( FOLIAGE );
		SetBlockAndTransparency( x-2, y, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x-2, y-1, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x-2, y+1, zz, b,  TRANSPARENCY_GREENERY );

		SetBlockAndTransparency( x+1, y+2+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+1,y-2-(x&1), zz, b,  TRANSPARENCY_GREENERY );

		SetBlockAndTransparency( x+2, y+2, zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+2, y-2, zz, b,  TRANSPARENCY_GREENERY );

		SetBlockAndTransparency( x+3, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
		SetBlockAndTransparency( x+3, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
	}
}


void h_Chunk::PlantGrass()
{
	//TODO - optomize
	short x, y, z, addr;
	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	{
		for( y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			for( z= H_CHUNK_HEIGHT - 2; z> 0; z-- )
				if ( GetBlock( x, y, z )->Type() != AIR )
					break;

			if( GetBlock( x, y, z )->Type() == SOIL )
				SetBlockAndTransparency( x, y, z, world_->NormalBlock( GRASS ), TRANSPARENCY_SOLID );
		}
	}
}

unsigned int h_Chunk::CalculateWaterBlockCount()
{
	unsigned int c= 0;
	for( unsigned int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
		if( blocks_[i]->Type() == WATER )
			c++;
	return c;
}

void h_Chunk::PrepareWaterBlocksCache( int needed_block_count )
{
	unsigned int final_buffer_size= needed_block_count * 5 / 4;
	water_blocks_data.initial_water_block_buffer_size= std::max( final_buffer_size, H_CHUNK_INITIAL_WATER_BLOCK_COUNT );
	water_blocks_data.initial_water_blocks= new
	h_LiquidBlock[ water_blocks_data.initial_water_block_buffer_size ];

	water_blocks_data.free_blocks_position= needed_block_count;
}

void h_Chunk::GenWaterBlocks()
{
	unsigned int c= CalculateWaterBlockCount();

	water_blocks_data.initial_water_block_buffer_size= std::max( c * 2, H_CHUNK_INITIAL_WATER_BLOCK_COUNT );
	water_blocks_data.initial_water_blocks= new
	h_LiquidBlock[ water_blocks_data.initial_water_block_buffer_size ];

	water_blocks_data.free_blocks_position= c;

	h_LiquidBlock* water_blocks= water_blocks_data.initial_water_blocks;

	short x, y, z, addr = 0;
	for( x= 0; x< H_CHUNK_WIDTH; x++ )
	{
		for( y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			for( z= 0; z< H_CHUNK_HEIGHT; z++, addr++ )
			{
				if( blocks_[ addr ]->Type() == WATER )
				{
					water_blocks->x_= x;
					water_blocks->y_= y;
					water_blocks->z_= z;
					blocks_[ addr ]= water_blocks;
					water_blocks->SetLiquidLevel(
						H_MAX_WATER_LEVEL + H_WATER_COMPRESSION_PER_BLOCK * ( H_SEA_LEVEL - water_blocks->z_ ) );
					water_blocks++;
				}
			}
		}
	}

	for( unsigned int i= 0; i< c; i++ )
		water_blocks_data.water_block_list.Add( water_blocks_data.initial_water_blocks + i );
}

h_LiquidBlock* h_Chunk::NewWaterBlock()
{
	h_LiquidBlock* b;
	if( water_blocks_data.free_blocks_position< water_blocks_data.initial_water_block_buffer_size )
	{
		water_blocks_data.free_blocks_position++;
		b= water_blocks_data.initial_water_blocks + water_blocks_data.free_blocks_position - 1;
		water_blocks_data.water_block_list.Add( b );
		return b;
	}
	else
	{
		water_blocks_data.water_block_list.Add( b= new h_LiquidBlock() );
		return b;
	}
}

void h_Chunk::DeleteWaterBlock( h_LiquidBlock* b )
{
	/*for( unsigned int i= 0; i <  water_blocks_data.water_block_list.Size(); i++ )
		if( water_blocks_data.water_block_list.Data()[i] == b )
		{
			water_blocks_data.water_block_list.Remove(i);
			break;
		}*/
	if( b > water_blocks_data.initial_water_blocks + water_blocks_data.initial_water_block_buffer_size ||
			b < water_blocks_data.initial_water_blocks )//if block outside of initial buffer
	{
		delete b;
	}
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

void h_Chunk::MakeLight()
{
	for( unsigned int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( unsigned int y= 0; y< H_CHUNK_WIDTH; y++ )
		{
			unsigned int addr= BlockAddr( x, y, H_CHUNK_HEIGHT-2 );
			unsigned int z;

			for( z= H_CHUNK_HEIGHT-2; z> 0; z--, addr-- )
			{
				if( blocks_[ addr ]->Type() != AIR )
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
				if( blocks_[ addr ]->Type() != AIR )
					break;
				sun_light_map_[ addr ]= H_MAX_SUN_LIGHT;
			}
			for( ; z > 0; z--, addr-- )
				sun_light_map_[ addr ]= 0;
		}
}

h_Chunk::h_Chunk( h_World* world, int longitude, int latitude )
	: world_(world)
	, longitude_(longitude), latitude_(latitude)
	, need_update_light_(false)
{
	GenChunk();
	PlantGrass();
	PlantTrees();
	GenWaterBlocks();
	MakeLight();

}

h_Chunk::h_Chunk( h_World* world, const HEXCHUNK_header* header, QDataStream& stream )
	: world_(world)
	, longitude_(header->longitude)
	, latitude_ (header->latitude )
	, need_update_light_(false)
{
	GenChunkFromFile( header, stream );
	MakeLight();
}

h_Chunk::~h_Chunk()
{
	//delete water blocks outside water_blocks_data.initial_water_blocks
	h_LiquidBlock* last_liquid_block_in_initial_buffer=
		water_blocks_data.initial_water_blocks + water_blocks_data.initial_water_block_buffer_size - 1;
	for( unsigned int i= 0; i< water_blocks_data.water_block_list.Size(); i++ )
	{
		h_LiquidBlock* b= water_blocks_data.water_block_list.Data()[i];
		if( b < water_blocks_data.initial_water_blocks || b > last_liquid_block_in_initial_buffer )
			delete b;
	}

	delete[] water_blocks_data.initial_water_blocks;
}

unsigned int h_Chunk::GetWaterColumnHeight( short x, short y, short z )
{
	unsigned int h= (z-1) * H_MAX_WATER_LEVEL;
	unsigned int addr= BlockAddr( x, y, z );
	while(  blocks_[addr]->Type() == WATER )
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
