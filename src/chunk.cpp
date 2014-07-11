#ifndef CHUNK_CPP
#define CHUNK_CPP

#include "chunk.hpp"
#include "world.hpp"
#include "math_lib/rand.h"

#define H_SEA_LEVEL  (H_CHUNK_HEIGHT/2 -4 )

float Noise2(const int x, const int y)   //range - [-1;1]
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /
           1073741823.5f - 1.0f;
}

float InterpolatedNoise(const short x, const short y)   //range - [-1;1]
{
    const short X = x >> 6;
    const short Y = y >> 6;

    const float noise[4]=
    {
        Noise2(X, Y),
        Noise2(X + 1, Y),
        Noise2(X + 1, Y + 1),
        Noise2(X, Y + 1)
    };

    const float dx = (float (x) - float (X << 6))*0.015625f;
    const float dy = (float (y) - float (Y << 6))*0.015625f;

    const float interp_x[2]=
    {
        dy * noise[3] + (1.0f - dy) * noise[0],
        dy * noise[2] + (1.0f - dy) * noise[1]
    };
    return interp_x[1] * dx + interp_x[0] * (1.0f - dx);
}

float FinalNoise(short x, short y)   //range [-1;1]  middle= 0
{
    float r = 0.5f * InterpolatedNoise(x, y);

    x <<= 1, y <<= 1;
    r += 0.25f * InterpolatedNoise(x, y);

    x <<= 1, y <<= 1;
    return (r += 0.125f * InterpolatedNoise(x, y));
}


bool h_Chunk::IsEdgeChunk()
{
    //return true;
    return longitude == world->Longitude() || latitude == world->Latitude() ||
           longitude == ( world->Longitude() + world->ChunkNumberX() - 1 ) ||
           latitude == ( world->Latitude() + world->ChunkNumberY() - 1 );
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
            h= H_CHUNK_HEIGHT/2 + short( 24.0f * FinalNoise( short( float( x + longitude * H_CHUNK_WIDTH ) * H_SPACE_SCALE_VECTOR_X  ),
                                         y + latitude * H_CHUNK_WIDTH ) );
            //if( longitude == -1 &&  latitude == -1 )h= 3;

            soil_h= 4 + short( 2.0f * FinalNoise( short( float( x + longitude * H_CHUNK_WIDTH ) * H_SPACE_SCALE_VECTOR_X ) * 4,
                                                  ( y + latitude * H_CHUNK_WIDTH ) * 4  ) );

            SetBlockAndTransparency( x, y, 0, world->NormalBlock( SPHERICAL_BLOCK), TRANSPARENCY_SOLID );
            SetBlockAndTransparency( x, y, H_CHUNK_HEIGHT-1, world->NormalBlock( SPHERICAL_BLOCK), TRANSPARENCY_SOLID );
            for( z= 1; z< h - soil_h; z++ )
            {
                transparency[ addr=BlockAddr( x, y, z )  ]= TRANSPARENCY_SOLID;
                blocks[ addr ]= world->NormalBlock( STONE );
            }

            for( ; z< h; z++ )
            {
                transparency[ addr=BlockAddr( x, y, z ) ]= TRANSPARENCY_SOLID;
                blocks[ addr ]= world->NormalBlock( SOIL );
            }

            //if( !( longitude == -1 && latitude == -1 ) )
            for( ; z<= H_SEA_LEVEL; z++ )
            {
                transparency[ addr=BlockAddr( x, y, z )  ]= TRANSPARENCY_LIQUID;
                blocks[ addr ]= world->NormalBlock( WATER );
            }

            for( ; z< H_CHUNK_HEIGHT-1; z++ )
            {
                transparency[ addr=BlockAddr( x, y, z )  ]= TRANSPARENCY_AIR;
                blocks[ addr ]= world->NormalBlock( AIR );
            }

        }
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
            SetBlock( tree_x, tree_y, h, world->NormalBlock( WOOD ) );

            if( h - tree_z > 1  )
            {
                SetTransparency( tree_x, tree_y + 1, h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x, tree_y + 1, h, world->NormalBlock( FOLIAGE ) );

                SetTransparency( tree_x, tree_y - 1, h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x, tree_y - 1, h, world->NormalBlock( FOLIAGE ) );

                SetTransparency( tree_x + 1, tree_y + ((tree_x+1)&1), h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x + 1, tree_y + ((tree_x+1)&1), h, world->NormalBlock( FOLIAGE ) );
                SetTransparency( tree_x + 1, tree_y - (tree_x&1), h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x + 1, tree_y - (tree_x&1), h, world->NormalBlock( FOLIAGE ) );

                SetTransparency( tree_x - 1, tree_y + ((tree_x+1)&1), h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x - 1, tree_y + ((tree_x+1)&1), h, world->NormalBlock( FOLIAGE ) );
                SetTransparency( tree_x - 1, tree_y - (tree_x&1), h, TRANSPARENCY_GREENERY );
                SetBlock( tree_x - 1, tree_y - (tree_x&1), h, world->NormalBlock( FOLIAGE ) );
            }
        }
        SetTransparency( tree_x, tree_y, h, TRANSPARENCY_GREENERY );
        SetBlock( tree_x, tree_y, h, world->NormalBlock( FOLIAGE ) );

    }
}


void h_Chunk::PlaneBigTree( short x, short y, short z )//local coordinates
{
    for( short zz= z; zz< z+7; zz++ )
    {
        SetBlockAndTransparency( x, y, zz, world->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
        SetBlockAndTransparency( x+1, y+((x+1)&1), zz, world->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
        SetBlockAndTransparency( x+1, y-(x&1), zz, world->NormalBlock( WOOD ),  TRANSPARENCY_SOLID );
    }
    for( short zz= z+7; zz< z+9; zz++ )
    {
        h_Block* b= world->NormalBlock( FOLIAGE );
        SetBlockAndTransparency( x, y, zz, b,  TRANSPARENCY_GREENERY );
        SetBlockAndTransparency( x+1, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
        SetBlockAndTransparency( x+1, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
    }


    for( short zz= z+2; zz< z+8; zz++ )
    {
        h_Block* b= world->NormalBlock( FOLIAGE );
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
        h_Block* b= world->NormalBlock( FOLIAGE );
        SetBlockAndTransparency( x, y+2, zz, b,  TRANSPARENCY_GREENERY );
        SetBlockAndTransparency( x, y-2, zz, b,  TRANSPARENCY_GREENERY );

        SetBlockAndTransparency( x-1, y-1-(x&1), zz, b,  TRANSPARENCY_GREENERY );
        SetBlockAndTransparency( x-1, y+1+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );

        SetBlockAndTransparency( x+3, y+((x+1)&1), zz, b,  TRANSPARENCY_GREENERY );
        SetBlockAndTransparency( x+3, y-(x&1), zz, b,  TRANSPARENCY_GREENERY );
    }

    for( short zz= z+4; zz< z+6; zz++ )
    {
        h_Block* b= world->NormalBlock( FOLIAGE );
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
    short x, y, z, addr;
    for( x= 0; x< H_CHUNK_WIDTH; x++ )
    {
        for( y= 0; y< H_CHUNK_WIDTH; y++ )
        {
            for( z= H_CHUNK_HEIGHT - 2; z> 0; z-- )
                if ( GetBlock( x, y, z )->Type() != AIR )
                    break;

            if( GetBlock( x, y, z )->Type() == SOIL )
            {
                transparency[ addr= z |
                                    ( y << H_CHUNK_HEIGHT_LOG2 ) |
                                    ( x << ( H_CHUNK_HEIGHT_LOG2 + H_CHUNK_WIDTH_LOG2 ) ) ]= TRANSPARENCY_SOLID;
                blocks[ addr ]= world->NormalBlock( GRASS );
            }

        }
    }
}

unsigned int h_Chunk::CalculateWaterBlockCount()
{
    unsigned int c= 0;
    for( unsigned int i= 0; i< H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT; i++ )
        if( blocks[i]->Type() == WATER )
            c++;
    return c;
}

void h_Chunk::GenWaterBlocks()
{
    unsigned int c= CalculateWaterBlockCount();

    water_blocks_data.initial_water_block_buffer_size= max( c * 2, H_CHUNK_INITIAL_WATER_BLOCK_COUNT );
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
                if( blocks[ addr ]->Type() == WATER )
                {
                    water_blocks->x= x;
                    water_blocks->y= y;
                    water_blocks->z= z;
                    blocks[ addr ]= water_blocks;
                    water_blocks->SetLiquidLevel(
                        H_MAX_WATER_LEVEL + H_WATER_COMPRESSION_PER_BLOCK * ( H_SEA_LEVEL - water_blocks->z ) );
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
    light_source_list.Add( s= new h_LightSource( type ) );
    s->x= x;
    s->y= y;
    s->z= z;
    return s;
}
void h_Chunk::DeleteLightSource( short x, short y, short z )
{
    h_LightSource* s= (h_LightSource*) GetBlock( x, y, z );

    m_Collection< h_LightSource* >::Iterator it( &light_source_list );
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
                if( blocks[ addr ]->Type() != AIR )
                    break;
                sun_light_map[ addr ]= H_MAX_SUN_LIGHT;
                fire_light_map[ addr ]= 0;
            }
            for( ; z > 0; z--, addr-- )
            {
                sun_light_map[ addr ]= 0;
                fire_light_map[ addr ]= 0;
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
                if( blocks[ addr ]->Type() != AIR )
                    break;
                sun_light_map[ addr ]= H_MAX_SUN_LIGHT;
            }
            for( ; z > 0; z--, addr-- )
                sun_light_map[ addr ]= 0;
        }
}

h_Chunk::h_Chunk( h_World* world, int longitude, int latitude )
{
    need_update_light= false;
    this->longitude= longitude, this->latitude= latitude;
    this->world= world;

    GenChunk();
    PlantGrass();
    PlantTrees();
    GenWaterBlocks();
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
    while(  blocks[addr]->Type() == WATER )
    {
        unsigned int level= ((h_LiquidBlock*)blocks[addr])->LiquidLevel();
        if( level > H_MAX_WATER_LEVEL )
            level= H_MAX_WATER_LEVEL;
        h+= level;
        addr++;
    }
    return h;
}



void h_Chunk::ReCalculateHeightmap()
{

    for( short y= 0; y< H_CHUNK_WIDTH; y++ )
        for( short x= 0; x< H_CHUNK_WIDTH; y++ )
        {
            int addr= (H_CHUNK_HEIGHT-2) | ( y << H_CHUNK_HEIGHT_LOG2 ) | ( x << ( H_CHUNK_HEIGHT_LOG2 + H_CHUNK_WIDTH_LOG2 ) );
            for( short z= H_CHUNK_HEIGHT-2; z>=0; z--, addr-- )
            {
                if( blocks[addr]->Type() != AIR )
                {
                    height_map[ x | (y<<H_CHUNK_WIDTH_LOG2) ]= z;
                    break;
                }
            }
        }
}
#endif//CHUNK_CPP
