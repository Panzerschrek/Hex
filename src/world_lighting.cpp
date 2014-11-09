#include "renderer/i_world_renderer.hpp"
#include "world.hpp"

unsigned char h_World::SunLightLevel( short x, short y, short z ) const
{
    return GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
           SunLightLevel( x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z );
}
unsigned char h_World::FireLightLevel( short x, short y, short z ) const
{
    return GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
           FireLightLevel( x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z );
}

void h_World::SetSunLightLevel( short x, short y, short z, unsigned char l )
{
    GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
    sun_light_map[ BlockAddr(  x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z ) ]= l;
}

void h_World::SetFireLightLevel( short x, short y, short z, unsigned char l )
{
    GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
    fire_light_map[ BlockAddr(  x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z ) ]= l;
}

short h_World::RelightBlockAdd( short x, short y, short z )
{
    unsigned char l= SunLightLevel( x, y, z );
    unsigned char fire_l= FireLightLevel( x, y, z );
    SetSunLightLevel( x, y, z, 0 );
    SetFireLightLevel( x, y, z, 0 );
    if( l == 0 && fire_l == 0 )
        return 1;

    short x_min, x_max, y_min, y_max, z_min, z_max;
    x_min= x + 1 - (short)(l);
    x_max= x - 1 + (short)(l);
    y_min= y + 1 - (short)(l);
    y_max= y - 1 + (short)(l);
    z_max= ClampZ( z - 1 + (short)(l) );

    short i, j, k;
    //remove all light
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
            for( k= z_max; k> 0; k-- )
            {
                SetSunLightLevel( i, j, k, 0 );
            }

    //sun shine in square
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
        {
            h_Chunk* ch= GetChunk( i>> H_CHUNK_WIDTH_LOG2, j>> H_CHUNK_WIDTH_LOG2 );
            short local_i= i & ( H_CHUNK_WIDTH-1), local_j= j & ( H_CHUNK_WIDTH-1);
            unsigned char* sun_light_map= ch->sun_light_map + BlockAddr( local_i, local_j, H_CHUNK_HEIGHT-2 );
            for( k= H_CHUNK_HEIGHT-2; k> 0; k--, sun_light_map-- )
            {
                if( ch->GetBlock( local_i, local_j, k )->Type() != AIR )
                    break;
                sun_light_map[0]= H_MAX_SUN_LIGHT;
            }
        }

    //secondary sun shine
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
            for( k= z_max; k> 0; k-- )
                AddSunLight_r( i, j, k,
                               SunLightLevel( i, j, k ) );
    //secondary sun shine from borders
    for( i= x_min; i<= x_max; i++ )
        for( k= z_max; k> 0; k-- )
        {
            AddSunLight_r( i, y_min-1, k,
                           SunLightLevel( i, y_min-1, k ) );
            AddSunLight_r( i, y_max+1, k,
                           SunLightLevel( i, y_max+1, k ) );
        }
    for( j= y_min; j<= y_max; j++ )
        for( k= z_max; k> 0; k-- )
        {
            AddSunLight_r( x_min-1, j, k,
                           SunLightLevel( x_min-1, j, k ) );
            AddSunLight_r( x_max+1, j, k,
                           SunLightLevel( x_max+1, j, k ) );
        }
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
            AddSunLight_r( i, j, z_max+1, SunLightLevel( i, j, z_max+1 ) );


    x_min= x + 1 - (short)(fire_l);
    x_max= x - 1 + (short)(fire_l);
    y_min= y + 1 - (short)(fire_l);
    y_max= y - 1 + (short)(fire_l);
    z_min= ClampZ( z + 1 - (short)(fire_l) );
    z_max= ClampZ( z - 1 + (short)(fire_l) );

    //zero fire light in cube
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
            for( k= z_min; k<= z_max; k++ )
                SetFireLightLevel( i, j, k, 0 );

    //secondary fire shine from borders
    for( i= x_min; i<= x_max; i++ )
        for( k= z_min; k<= z_max; k++ )
        {
            AddFireLight_r( i, y_min-1, k,
                            FireLightLevel( i, y_min-1, k ) );
            AddFireLight_r( i, y_max+1, k,
                            FireLightLevel( i, y_max+1, k ) );
        }
    for( j= y_min; j<= y_max; j++ )
        for( k= z_min; k<= z_max; k++ )
        {
            AddFireLight_r( x_min-1, j, k,
                            FireLightLevel( x_min-1, j, k ) );
            AddFireLight_r( x_max+1, j, k,
                            FireLightLevel( x_max+1, j, k ) );
        }
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
        {
            AddFireLight_r( i, j, z_min-1,
                            FireLightLevel( i, j, z_min-1 ) );
            AddFireLight_r( i, j, z_max+1,
                            FireLightLevel( i, j, z_max+1 ) );
        }


    //shining from kight sources in cube
    ShineFireLight( x_min, y_min, z_min, x_max, y_max, z_max );

    return (short)max( l, fire_l );
}

void h_World::RelightBlockRemove( short x, short y, short z )
{
    short x1, y1, z1, z0;
    h_Chunk* ch= GetChunk( x>> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 );
    x1= x& ( H_CHUNK_WIDTH-1);
    y1= y& ( H_CHUNK_WIDTH-1);
    unsigned char* lightmap= &ch->sun_light_map[ BlockAddr( x1, y1, H_CHUNK_HEIGHT-2 ) ];
    for( z1= H_CHUNK_HEIGHT-2; z1>0 ; z1--, lightmap-- )
    {
        if( ch->GetBlock( x1, y1, z1 )->Type() != AIR )
            break;
        *lightmap= H_MAX_SUN_LIGHT;
    }
    z0= z1;
    for( z1= z; z1> z0; z1-- )
    {
        AddSunLight_r( x, y, z1, SunLightLevel( x, y, z1) );
    }


    AddSunLight_r( x, y, z+ 1, SunLightLevel(x, y, z+ 1) );
    AddSunLight_r( x, y, z-1, SunLightLevel(x, y, z-1) );
    AddSunLight_r( x, y+1, z, SunLightLevel(x, y+1, z) );
    AddSunLight_r( x, y-1, z, SunLightLevel(x, y-1, z) );
    AddSunLight_r( x+1, y+((1+x)&1), z, SunLightLevel(x+1, y+((1+x)&1), z) );
    AddSunLight_r( x+1, y- (x&1), z, SunLightLevel(x+1, y- (x&1), z) );
    AddSunLight_r( x-1, y+((1+x)&1), z, SunLightLevel(x-1, y+((1+x)&1), z) );
    AddSunLight_r( x-1, y- (x&1), z , SunLightLevel(x-1, y- (x&1), z) );

    AddFireLight_r( x, y, z+ 1, FireLightLevel(x, y, z+ 1) );
    AddFireLight_r( x, y, z-1, FireLightLevel(x, y, z-1) );
    AddFireLight_r( x, y+1, z, FireLightLevel(x, y+1, z) );
    AddFireLight_r( x, y-1, z, FireLightLevel(x, y-1, z) );
    AddFireLight_r( x+1, y+((1+x)&1), z, FireLightLevel(x+1, y+((1+x)&1), z) );
    AddFireLight_r( x+1, y- (x&1), z, FireLightLevel(x+1, y- (x&1), z) );
    AddFireLight_r( x-1, y+((1+x)&1), z, FireLightLevel(x-1, y+((1+x)&1), z) );
    AddFireLight_r( x-1, y- (x&1), z , FireLightLevel(x-1, y- (x&1), z) );
}


//table for calculating vertex light without division ( must be faster, but not precise )
static const unsigned int vertex_light_div_table[]=
{
    0, 65536 * (13+1)/1, 65536 * (13+2)/2, 65536 * (13+3)/3, 65536 * (13+4)/4, 65536 * (13+5)/5, 65536 * (13+6)/6
};

void h_World::GetForwardVertexLight( short x, short y, short z, unsigned char* out_light ) const
{
    unsigned short block_count= 0;
    unsigned char light[2]= { 0, 0 };
    short  x1, y1;
    const h_Chunk* ch;
    unsigned int addr;

    //current block
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

    //forwar block
    y1= y+1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

    //forward right block
    x1= x + 1;
    y1= y + ((x+1)&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

   // out_light[0]= (unsigned char)( ( ((unsigned short)(light[0])) * ( 13 + block_count ) ) / block_count  );
   // out_light[1]= (unsigned char)( ( ((unsigned short)(light[1])) * ( 13 + block_count ) ) / block_count  );
    out_light[0]= ( ( (unsigned int)light[0] ) * vertex_light_div_table[ block_count ] )>> 16;
    out_light[1]= ( ( (unsigned int)light[1] ) * vertex_light_div_table[ block_count ] )>> 16;
}

void h_World::GetBackVertexLight( short x, short y, short z, unsigned char* out_light ) const
{
    unsigned short block_count= 0;
    unsigned char light[2]= { 0, 0 };
    short  x1, y1;
    const h_Chunk* ch;
    unsigned int addr;

    //current block
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

    //back block
    y1= y-1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

    //back left block
    x1= x - 1;
    y1= y - (x&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }
    addr++;
    if( ch->transparency[addr] != TRANSPARENCY_SOLID )
    {
        light[0]+= ch->sun_light_map[ addr ];
        light[1]+= ch->fire_light_map[ addr ];
        block_count++;
    }

    //out_light[0]= (unsigned char)( ( ((unsigned short)(light[0])) * ( 13 + block_count ) ) / block_count  );
    //out_light[1]= (unsigned char)( ( ((unsigned short)(light[1])) * ( 13 + block_count ) ) / block_count  );
	out_light[0]= ( ( (unsigned int)light[0] ) * vertex_light_div_table[ block_count ] )>> 16;
    out_light[1]= ( ( (unsigned int)light[1] ) * vertex_light_div_table[ block_count ] )>> 16;
}


void h_World::LightWorld()
{
    //main chunks. used fast lighting functions ( without coordinate clamping )
    for( unsigned int i= 1; i< chunk_number_x - 1; i++ )
        for( unsigned int j= 1; j< chunk_number_y - 1; j++ )
        {
            h_Chunk* ch= GetChunk( i, j );
            short X= i * H_CHUNK_WIDTH, Y= j * H_CHUNK_WIDTH;
            for( short x= 0; x< H_CHUNK_WIDTH; x++ )
                for( short y= 0; y< H_CHUNK_WIDTH; y++ )
                    for( short z= 1; z< H_CHUNK_HEIGHT - 1; z++ )
                        AddSunLight_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

			int fire_light_count= ch->GetLightSourceList()->Size();
			auto fire_lights= ch->GetLightSourceList()->Data();
			for( int l= 0; l< fire_light_count; l++ )
				AddFireLight_r( X+fire_lights[l]->x, Y+fire_lights[l]->y, fire_lights[l]->z, fire_lights[l]->LightLevel() );
        }

    //north and south chunks
    for( unsigned int k= 0; k< 2; k++ )
    {
        for( unsigned int i= 0; i< chunk_number_x; i++ )
        {
            h_Chunk* ch= GetChunk( i, k * (chunk_number_y-1) );
            short X= i * H_CHUNK_WIDTH, Y=  H_CHUNK_WIDTH * k * (chunk_number_y-1) ;
            for( short x= 0; x< H_CHUNK_WIDTH; x++ )
                for( short y= 0; y< H_CHUNK_WIDTH; y++ )
                    for( short z= 1; z< H_CHUNK_HEIGHT-1; z++ )
                        AddSunLightSafe_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

			int fire_light_count= ch->GetLightSourceList()->Size();
			auto fire_lights= ch->GetLightSourceList()->Data();
			for( int l= 0; l< fire_light_count; l++ )
				AddFireLightSafe_r( X+fire_lights[l]->x, Y+fire_lights[l]->y, fire_lights[l]->z, fire_lights[l]->LightLevel() );
        }
    }
    //east and west chunks
    for( unsigned int k= 0; k< 2; k++ )
    {
        for( unsigned int j= 0; j< chunk_number_y; j++ )
        {
            h_Chunk* ch= GetChunk( k*( chunk_number_x - 1), j );
            short X= k*( chunk_number_x - 1) * H_CHUNK_WIDTH, Y=  j * H_CHUNK_WIDTH;
            for( short x= 0; x< H_CHUNK_WIDTH; x++ )
                for( short y= 0; y< H_CHUNK_WIDTH; y++ )
                    for( short z= 1; z< H_CHUNK_HEIGHT-1; z++ )
                        AddSunLightSafe_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

			int fire_light_count= ch->GetLightSourceList()->Size();
			auto fire_lights= ch->GetLightSourceList()->Data();
			for( int l= 0; l< fire_light_count; l++ )
				AddFireLightSafe_r( X+fire_lights[l]->x, Y+fire_lights[l]->y, fire_lights[l]->z, fire_lights[l]->LightLevel() );
        }
    }
}

void h_World::AddSunLight_r( short x, short y, short z, unsigned char l )
{
    h_Chunk* ch;
    short x1, y1;
    unsigned int addr;

    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    ch->sun_light_map[ addr ]= l;

    if( l <= 1 )
        return;
    unsigned char l1= l-1;

    //upper block
    addr++;
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x, y, z + 1, l1 );
    //lower block
    addr-= 2;
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x, y, z - 1, l1 );
    //forward block
    y1= y + 1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x, y1, z, l1 );
    //back block
    y1= y - 1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x, y1, z, l1 );
    //forwad right
    x1= x+1;
    y1= y +((x+1)&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x1, y1, z, l1 );
    //back right
    x1= x+1;
    y1= y -(x&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x1, y1, z, l1 );
    //forwad left
    x1= x-1;
    y1= y +((x+1)&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x1, y1, z, l1 );
    //back left
    x1= x-1;
    y1= y -(x&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLight_r( x1, y1, z, l1 );
}

void h_World::AddSunLightSafe_r( short x, short y, short z, unsigned char l )
{
    h_Chunk* ch;
    short x1, y1, z1;
    unsigned int addr;

    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    ch->sun_light_map[ addr ]= l;

    if( l <= (unsigned char)1 )
        return;
    unsigned char l1= l-1;

    //upper block
    z1= z+1;
    addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x, y, z1, l1 );
    //lower block
    z1= z-1;
    addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x, y, z1, l1 );
    //forward block
    y1= ClampY(y + 1);
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x, y1, z, l1 );
    //back block
    y1= ClampY( y - 1 );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x, y1, z, l1 );
    //forwad right
    x1= ClampX( x+1 );
    y1= ClampY( y +((x+1)&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x1, y1, z, l1 );
    //back right
    x1= ClampX( x+1 );
    y1= ClampY( y -(x&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x1, y1, z, l1 );
    //forwad left
    x1= ClampX( x-1 );
    y1= ClampY( y +((x+1)&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x1, y1, z, l1 );
    //back left
    x1= ClampX( x-1 );
    y1= ClampY( y -(x&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->sun_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddSunLightSafe_r( x1, y1, z, l1 );
}





void h_World::AddFireLight_r( short x, short y, short z, unsigned char l )
{
    h_Chunk* ch;
    short x1, y1;
    unsigned int addr;

    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    ch->fire_light_map[ addr ]= l;

    if( l <= 1 )
        return;
    unsigned char l1= l-1;

    //upper block
    addr++;
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x, y, z + 1, l1 );
    //lower block
    addr-= 2;
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x, y, z - 1, l1 );
    //forward block
    y1= y + 1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x, y1, z, l1 );
    //back block
    y1= y - 1;
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x, y1, z, l1 );
    //forwad right
    x1= x+1;
    y1= y +((x+1)&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x1, y1, z, l1 );
    //back right
    x1= x+1;
    y1= y -(x&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x1, y1, z, l1 );
    //forwad left
    x1= x-1;
    y1= y +((x+1)&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x1, y1, z, l1 );
    //back left
    x1= x-1;
    y1= y -(x&1);
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLight_r( x1, y1, z, l1 );
}

void h_World::AddFireLightSafe_r( short x, short y, short z, unsigned char l )
{
    h_Chunk* ch;
    short x1, y1, z1;
    unsigned int addr;

    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
    ch->fire_light_map[ addr ]= l;

    if( l <= 1 )
        return;
    unsigned char l1= l-1;

    //upper block
    z1= z+1;
    addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x, y, z1, l1 );
    //lower block
    z1= z-1;
    addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x, y, z1, l1 );
    //forward block
    y1= ClampY(y + 1);
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x, y1, z, l1 );
    //back block
    y1= ClampY( y - 1 );
    ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x, y1, z, l1 );
    //forwad right
    x1= ClampX( x+1 );
    y1= ClampY( y +((x+1)&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x1, y1, z, l1 );
    //back right
    x1= ClampX( x+1 );
    y1= ClampY( y -(x&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x1, y1, z, l1 );
    //forwad left
    x1= ClampX( x-1 );
    y1= ClampY( y +((x+1)&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x1, y1, z, l1 );
    //back left
    x1= ClampX( x-1 );
    y1= ClampY( y -(x&1) );
    ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
    addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
    if( ch->fire_light_map[addr] < l1 && ch->transparency[addr] != TRANSPARENCY_SOLID )
        AddFireLightSafe_r( x1, y1, z, l1 );
}


void h_World::ShineFireLight( short x_min, short y_min, short z_min, short x_max, short y_max, short z_max )
{
    x_min>>= H_CHUNK_WIDTH_LOG2;
    x_max>>= H_CHUNK_WIDTH_LOG2;
    y_min>>= H_CHUNK_WIDTH_LOG2;
    y_max>>= H_CHUNK_WIDTH_LOG2;

    short i, j;
    for( i= x_min; i<= x_max; i++ )
        for( j= y_min; j<= y_max; j++ )
        {
            auto light_source_list= GetChunk( i, j )->GetLightSourceList();
            h_LightSource** s= light_source_list->Data();
            h_LightSource** s_end= s + light_source_list->Size();
            short X, Y;
            X= i<< H_CHUNK_WIDTH_LOG2;
            Y= j<< H_CHUNK_WIDTH_LOG2;
            for( ; s!= s_end; s++ )
                AddFireLight_r( s[0]->x + X, s[0]->y + Y, s[0]->z, s[0]->LightLevel() );
        }
}


void h_World::AddLightToBorderChunk( unsigned int X, unsigned int Y )
{
    h_Chunk* ch= GetChunk( X, Y );

    short x= X * H_CHUNK_WIDTH;
    short y= Y * H_CHUNK_WIDTH;

    for( short i= 0; i< H_CHUNK_WIDTH; i++ )
        for( short j= 0; j< H_CHUNK_WIDTH; j++ )
            for( short k= 1; k< H_CHUNK_HEIGHT-1; k++ )
                AddSunLightSafe_r( x+i, y+j, k, ch->SunLightLevel( i, j, k ) );

	//add fire lights to border chunk
	int fire_light_count= ch->GetLightSourceList()->Size();
	auto fire_lights= ch->GetLightSourceList()->Data();
	for( int l= 0; l< fire_light_count; l++ )
		AddFireLightSafe_r( x+fire_lights[l]->x, y+fire_lights[l]->y, fire_lights[l]->z, fire_lights[l]->LightLevel() );

}


void h_World::RelightWaterModifedChunksLight()
{
    unsigned int chunk_count= 0;
    h_Chunk* ch;
    short X, Y;
    for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
        for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
        {
            ch= GetChunk( i, j );
            if( ch->need_update_light )
                chunk_count++;
        }

    for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
        for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
        {
            ch= GetChunk( i, j );
            if( ch->need_update_light )
            {
                if(  phys_processes_rand.Rand()  <= phys_processes_rand.max_rand/ chunk_count )//chance of one chunk water updating per one phys tick
                {
                    X= i<<H_CHUNK_WIDTH_LOG2;
                    Y= j<<H_CHUNK_WIDTH_LOG2;
                    ch->SunRelight();//zero light in chunk and add vertical sun light
                    for( short x= 0; x< H_CHUNK_WIDTH; x++ )
                        for( short y=0; y< H_CHUNK_WIDTH; y++ )
                            for( short z= 1; z< H_CHUNK_HEIGHT-1; z++ )
                            {
                                AddSunLight_r( X+x, Y+y, z, SunLightLevel( X+x, Y+y, z) );
                            }
                    for( short x= 0; x< H_CHUNK_WIDTH; x++ )
                        for( short z= 1; z< H_CHUNK_HEIGHT-1; z++ )
                        {
                            AddSunLight_r( X+x, Y-1, z, SunLightLevel( X+x, Y-1, z) );
                            AddSunLight_r( X+x, Y+H_CHUNK_WIDTH, z, SunLightLevel( X+x, Y+H_CHUNK_WIDTH, z) );
                        }
                    for( short y= 0; y< H_CHUNK_WIDTH; y++ )
                        for( short z= 1; z< H_CHUNK_HEIGHT-1; z++ )
                        {
                            AddSunLight_r( X-1, Y+y, z, SunLightLevel( X-1, Y+y, z) );
                            AddSunLight_r( X+H_CHUNK_WIDTH, Y+y, z, SunLightLevel( X+H_CHUNK_WIDTH, Y+y, z) );
                        }
                    ch->need_update_light= false;

					if( renderer != nullptr )
					{
                    /*emit ChunkUpdated( i, j );
                    emit ChunkUpdated( i+1, j+1 );
                    emit ChunkUpdated( i+1, j-1 );
                    emit ChunkUpdated( i-1, j+1 );
                    emit ChunkUpdated( i-1, j-1 );
                    emit ChunkWaterUpdated( i, j );*/
						renderer->UpdateChunk( i, j );
						renderer->UpdateChunk( i+1, j+1 );
						renderer->UpdateChunk( i+1, j-1 );
						renderer->UpdateChunk( i-1, j+1 );
						renderer->UpdateChunk( i-1, j-1 );
						renderer->UpdateChunkWater( i, j );
					}
                }//if rand
            }//if need update light
        }

}
