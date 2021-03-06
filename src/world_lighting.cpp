#include "renderer/i_world_renderer.hpp"
#include "world.hpp"

// Table for replacement of division in vertex light calculation.
// This method not precise, but fast.
static const unsigned int g_vertex_light_div_shift= 16;
static const unsigned int g_vertex_light_div_table_multiplier= 1 << g_vertex_light_div_shift;
static const unsigned int g_vertex_light_div_table[]=
{
	0,
	g_vertex_light_div_table_multiplier * (13+1)/1,
	g_vertex_light_div_table_multiplier * (13+2)/2,
	g_vertex_light_div_table_multiplier * (13+3)/3,
	g_vertex_light_div_table_multiplier * (13+4)/4,
	g_vertex_light_div_table_multiplier * (13+5)/5,
	g_vertex_light_div_table_multiplier * (13+6)/6,
};

unsigned char h_World::SunLightLevel( int x, int y, int z ) const
{
	return
		GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
		SunLightLevel( x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z );
}
unsigned char h_World::FireLightLevel( int x, int y, int z ) const
{
	return
	GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
	FireLightLevel( x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z );
}

void h_World::GetForwardVertexLight( int x, int y, int z, unsigned char* out_light ) const
{
	unsigned int block_count= 0;
	unsigned char light[2]= { 0, 0 };
	int  x1, y1;
	const h_Chunk* ch;
	unsigned int addr;

	//current block
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	//forwar block
	y1= y+1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	//forward right block
	x1= x + 1;
	y1= y + ((x+1)&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	out_light[0]= ( ( (unsigned int)light[0] ) * g_vertex_light_div_table[ block_count ] )>> g_vertex_light_div_shift;
	out_light[1]= ( ( (unsigned int)light[1] ) * g_vertex_light_div_table[ block_count ] )>> g_vertex_light_div_shift;
}

void h_World::GetBackVertexLight( int x, int y, int z, unsigned char* out_light ) const
{
	unsigned int block_count= 0;
	unsigned char light[2]= { 0, 0 };
	int  x1, y1;
	const h_Chunk* ch;
	unsigned int addr;

	//current block
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	//back block
	y1= y-1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	//back left block
	x1= x - 1;
	y1= y - (x&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1& (H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}
	addr++;
	if( ( ch->transparency_[addr] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_SOLID )
	{
		light[0]+= ch->sun_light_map_ [ addr ];
		light[1]+= ch->fire_light_map_[ addr ];
		block_count++;
	}

	out_light[0]= ( ( (unsigned int)light[0] ) * g_vertex_light_div_table[ block_count ] )>> g_vertex_light_div_shift;
	out_light[1]= ( ( (unsigned int)light[1] ) * g_vertex_light_div_table[ block_count ] )>> g_vertex_light_div_shift;
}

void h_World::SetSunLightLevel( int x, int y, int z, unsigned char l )
{
	GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
	sun_light_map_[ BlockAddr(  x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z ) ]= l;
}

void h_World::SetFireLightLevel( int x, int y, int z, unsigned char l )
{
	GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
	fire_light_map_[ BlockAddr(  x& (H_CHUNK_WIDTH-1), y& (H_CHUNK_WIDTH-1), z ) ]= l;
}

void h_World::LightWorld()
{
	//main chunks. used fast lighting functions ( without coordinate clamping )
	for( unsigned int i= 1; i< chunk_number_x_ - 1; i++ )
	for( unsigned int j= 1; j< chunk_number_y_ - 1; j++ )
	{
		h_Chunk* ch= GetChunk( i, j );
		int X= i * H_CHUNK_WIDTH, Y= j * H_CHUNK_WIDTH;
		for( int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( int y= 0; y< H_CHUNK_WIDTH; y++ )
		for( int z= 1; z< H_CHUNK_HEIGHT - 1; z++ )
			AddSunLight_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

		const std::vector< h_LightSource* >& light_sources= ch->GetLightSourceList();
		for( const h_LightSource* source : light_sources )
			AddFireLight_r( X + source->x_, Y + source->y_, source->z_, source->LightLevel() );
	}

	//north and south chunks
	for( unsigned int k= 0; k< 2; k++ )
	for( unsigned int i= 0; i< chunk_number_x_; i++ )
	{
		h_Chunk* ch= GetChunk( i, k * (chunk_number_y_-1) );
		int X= i * H_CHUNK_WIDTH, Y=  H_CHUNK_WIDTH * k * (chunk_number_y_-1) ;
		for( int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( int y= 0; y< H_CHUNK_WIDTH; y++ )
		for( int z= 1; z< H_CHUNK_HEIGHT-1; z++ )
			AddSunLightSafe_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

		const std::vector< h_LightSource* >& light_sources= ch->GetLightSourceList();
		for( const h_LightSource* source : light_sources )
			AddFireLightSafe_r( X + source->x_, Y + source->y_, source->z_, source->LightLevel() );
	}

	//east and west chunks
	for( unsigned int k= 0; k< 2; k++ )
	for( unsigned int j= 0; j< chunk_number_y_; j++ )
	{
		h_Chunk* ch= GetChunk( k*( chunk_number_x_ - 1), j );
		int X= k*( chunk_number_x_ - 1) * H_CHUNK_WIDTH, Y=  j * H_CHUNK_WIDTH;
		for( int x= 0; x< H_CHUNK_WIDTH; x++ )
		for( int y= 0; y< H_CHUNK_WIDTH; y++ )
		for( int z= 1; z< H_CHUNK_HEIGHT-1; z++ )
			AddSunLightSafe_r( X+x, Y+y, z, ch->SunLightLevel(x,y,z) );

		const std::vector< h_LightSource* >& light_sources= ch->GetLightSourceList();
		for( const h_LightSource* source : light_sources )
			AddFireLightSafe_r( X + source->x_, Y + source->y_, source->z_, source->LightLevel() );
	}
}

int h_World::RelightBlockAdd( int x, int y, int z )
{
	unsigned char l= SunLightLevel( x, y, z );
	unsigned char fire_l= FireLightLevel( x, y, z );
	SetSunLightLevel( x, y, z, 0 );
	SetFireLightLevel( x, y, z, 0 );
	if( l == 0 && fire_l == 0 )
		return 1;

	int x_min, x_max, y_min, y_max, z_min, z_max;
	x_min= x + 1 - (int)(l);
	x_max= x - 1 + (int)(l);
	y_min= y + 1 - (int)(l);
	y_max= y - 1 + (int)(l);
	z_max= ClampZ( z - 1 + (int)(l) );

	int i, j, k;
	//remove all light
	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	for( k= z_max; k> 0; k-- )
		SetSunLightLevel( i, j, k, 0 );

	//sun shine in square
	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	{
		h_Chunk* ch= GetChunk( i>> H_CHUNK_WIDTH_LOG2, j>> H_CHUNK_WIDTH_LOG2 );
		int local_i= i & ( H_CHUNK_WIDTH-1), local_j= j & ( H_CHUNK_WIDTH-1);
		unsigned char* sun_light_map= ch->sun_light_map_ + BlockAddr( local_i, local_j, H_CHUNK_HEIGHT-2 );

		unsigned int addr= BlockAddr( local_i, local_j, 0 );
		for( k= H_CHUNK_HEIGHT-2; k> 0; k--, sun_light_map-- )
		{
			if( !( ch->transparency_[ addr + k ] & H_DIRECT_SUN_LIGHT_TRANSPARENCY_BIT ) )
				break;
			sun_light_map[0]= H_MAX_SUN_LIGHT;
		}
	}

	//secondary sun shine
	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	for( k= z_max; k> 0; k-- )
		AddSunLight_r( i, j, k, SunLightLevel( i, j, k ) );

	//secondary sun shine from borders
	for( i= x_min; i<= x_max; i++ )
	for( k= z_max; k> 0; k-- )
	{
		AddSunLight_r( i, y_min-1, k, SunLightLevel( i, y_min-1, k ) );
		AddSunLight_r( i, y_max+1, k, SunLightLevel( i, y_max+1, k ) );
	}

	for( j= y_min; j<= y_max; j++ )
	for( k= z_max; k> 0; k-- )
	{
		AddSunLight_r( x_min-1, j, k, SunLightLevel( x_min-1, j, k ) );
		AddSunLight_r( x_max+1, j, k, SunLightLevel( x_max+1, j, k ) );
	}

	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
		AddSunLight_r( i, j, z_max+1, SunLightLevel( i, j, z_max+1 ) );


	x_min= x + 1 - (int)(fire_l);
	x_max= x - 1 + (int)(fire_l);
	y_min= y + 1 - (int)(fire_l);
	y_max= y - 1 + (int)(fire_l);
	z_min= ClampZ( z + 1 - (int)(fire_l) );
	z_max= ClampZ( z - 1 + (int)(fire_l) );

	//zero fire light in cube
	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	for( k= z_min; k<= z_max; k++ )
		SetFireLightLevel( i, j, k, 0 );

	//secondary fire shine from borders
	for( i= x_min; i<= x_max; i++ )
	for( k= z_min; k<= z_max; k++ )
	{
		AddFireLight_r( i, y_min-1, k, FireLightLevel( i, y_min-1, k ) );
		AddFireLight_r( i, y_max+1, k, FireLightLevel( i, y_max+1, k ) );
	}

	for( j= y_min; j<= y_max; j++ )
	for( k= z_min; k<= z_max; k++ )
	{
		AddFireLight_r( x_min-1, j, k, FireLightLevel( x_min-1, j, k ) );
		AddFireLight_r( x_max+1, j, k, FireLightLevel( x_max+1, j, k ) );
	}

	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	{
		AddFireLight_r( i, j, z_min-1, FireLightLevel( i, j, z_min-1 ) );
		AddFireLight_r( i, j, z_max+1, FireLightLevel( i, j, z_max+1 ) );
	}

	//shining from kight sources in cube
	ShineFireLight( x_min, y_min, z_min, x_max, y_max, z_max );

	return (int)std::max( l, fire_l );
}

void h_World::RelightBlockRemove( int x, int y, int z )
{
	int x1, y1, z1, z0;
	h_Chunk* ch= GetChunk( x>> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 );
	x1= x& ( H_CHUNK_WIDTH-1);
	y1= y& ( H_CHUNK_WIDTH-1);
	unsigned char* lightmap= &ch->sun_light_map_[ BlockAddr( x1, y1, H_CHUNK_HEIGHT-2 ) ];

	unsigned int addr= BlockAddr( x1, y1, 0 );
	for( z1= H_CHUNK_HEIGHT-2; z1>0 ; z1--, lightmap-- )
	{
		if( !( ch->transparency_[ addr + z1 ] & H_DIRECT_SUN_LIGHT_TRANSPARENCY_BIT ) )
			break;
		*lightmap= H_MAX_SUN_LIGHT;
	}
	z0= z1;
	for( z1= z; z1> z0; z1-- )
	{
		AddSunLight_r( x, y, z1, SunLightLevel( x, y, z1) );
	}

	AddSunLight_r( x, y, z, SunLightLevel(x, y, z) );

	/*
	AddSunLight_r( x, y, z+ 1, SunLightLevel(x, y, z+ 1) );
	AddSunLight_r( x, y, z-1, SunLightLevel(x, y, z-1) );
	AddSunLight_r( x, y+1, z, SunLightLevel(x, y+1, z) );
	AddSunLight_r( x, y-1, z, SunLightLevel(x, y-1, z) );
	AddSunLight_r( x+1, y+((1+x)&1), z, SunLightLevel(x+1, y+((1+x)&1), z) );
	AddSunLight_r( x+1, y- (x&1), z, SunLightLevel(x+1, y- (x&1), z) );
	AddSunLight_r( x-1, y+((1+x)&1), z, SunLightLevel(x-1, y+((1+x)&1), z) );
	AddSunLight_r( x-1, y- (x&1), z , SunLightLevel(x-1, y- (x&1), z) );
	*/

	AddFireLight_r( x, y, z, FireLightLevel(x, y, z) );

	/*
	AddFireLight_r( x, y, z+ 1, FireLightLevel(x, y, z+ 1) );
	AddFireLight_r( x, y, z-1, FireLightLevel(x, y, z-1) );
	AddFireLight_r( x, y+1, z, FireLightLevel(x, y+1, z) );
	AddFireLight_r( x, y-1, z, FireLightLevel(x, y-1, z) );
	AddFireLight_r( x+1, y+((1+x)&1), z, FireLightLevel(x+1, y+((1+x)&1), z) );
	AddFireLight_r( x+1, y- (x&1), z, FireLightLevel(x+1, y- (x&1), z) );
	AddFireLight_r( x-1, y+((1+x)&1), z, FireLightLevel(x-1, y+((1+x)&1), z) );
	AddFireLight_r( x-1, y- (x&1), z , FireLightLevel(x-1, y- (x&1), z) );
	*/
}

void h_World::AddSunLight_r( int x, int y, int z, unsigned char l )
{
	h_Chunk* ch;
	int x1, y1;
	unsigned int addr;

	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	ch->sun_light_map_[ addr ]= l;

	if( l <= 1 ||
		( ch->transparency_[addr] & H_SECONDARY_SUN_LIGHT_TRANSPARENCY_BIT ) == 0 )
		return;
	unsigned char l1= l-1;

	//upper block
	addr++;
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x, y, z + 1, l1 );
	//lower block
	addr-= 2;
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x, y, z - 1, l1 );
	//forward block
	y1= y + 1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x, y1, z, l1 );
	//back block
	y1= y - 1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x, y1, z, l1 );
	//forwad right
	x1= x+1;
	y1= y +((x+1)&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x1, y1, z, l1 );
	//back right
	x1= x+1;
	y1= y -(x&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x1, y1, z, l1 );
	//forwad left
	x1= x-1;
	y1= y +((x+1)&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x1, y1, z, l1 );
	//back left
	x1= x-1;
	y1= y -(x&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLight_r( x1, y1, z, l1 );
}

void h_World::AddFireLight_r( int x, int y, int z, unsigned char l )
{
	h_Chunk* ch;
	int x1, y1;
	unsigned int addr;

	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	ch->fire_light_map_[ addr ]= l;

	if( l <= 1 ||
		( ch->transparency_[addr] & H_FIRE_LIGHT_TRANSPARENCY_BIT ) == 0 )
		return;
	unsigned char l1= l-1;

	//upper block
	addr++;
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x, y, z + 1, l1 );
	//lower block
	addr-= 2;
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x, y, z - 1, l1 );
	//forward block
	y1= y + 1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x, y1, z, l1 );
	//back block
	y1= y - 1;
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x, y1, z, l1 );
	//forwad right
	x1= x+1;
	y1= y +((x+1)&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x1, y1, z, l1 );
	//back right
	x1= x+1;
	y1= y -(x&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x1, y1, z, l1 );
	//forwad left
	x1= x-1;
	y1= y +((x+1)&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x1, y1, z, l1 );
	//back left
	x1= x-1;
	y1= y -(x&1);
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLight_r( x1, y1, z, l1 );
}

void h_World::AddSunLightSafe_r( int x, int y, int z, unsigned char l )
{
	h_Chunk* ch;
	int x1, y1, z1;
	unsigned int addr;

	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	ch->sun_light_map_[ addr ]= l;

	if( l <= 1 ||
		( ch->transparency_[addr] & H_SECONDARY_SUN_LIGHT_TRANSPARENCY_BIT ) == 0 )
		return;
	unsigned char l1= l-1;

	//upper block
	z1= z+1;
	addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x, y, z1, l1 );
	//lower block
	z1= z-1;
	addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x, y, z1, l1 );
	//forward block
	y1= ClampY(y + 1);
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x, y1, z, l1 );
	//back block
	y1= ClampY( y - 1 );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x, y1, z, l1 );
	//forwad right
	x1= ClampX( x+1 );
	y1= ClampY( y +((x+1)&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x1, y1, z, l1 );
	//back right
	x1= ClampX( x+1 );
	y1= ClampY( y -(x&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x1, y1, z, l1 );
	//forwad left
	x1= ClampX( x-1 );
	y1= ClampY( y +((x+1)&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x1, y1, z, l1 );
	//back left
	x1= ClampX( x-1 );
	y1= ClampY( y -(x&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->sun_light_map_[addr] < l1 )
		AddSunLightSafe_r( x1, y1, z, l1 );
}

void h_World::AddFireLightSafe_r( int x, int y, int z, unsigned char l )
{
	h_Chunk* ch;
	int x1, y1, z1;
	unsigned int addr;

	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y >> H_CHUNK_WIDTH_LOG2 );
	ch->fire_light_map_[ addr ]= l;

	if( l <= 1 ||
		( ch->transparency_[addr] & H_FIRE_LIGHT_TRANSPARENCY_BIT ) == 0 )
		return;
	unsigned char l1= l-1;

	//upper block
	z1= z+1;
	addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x, y, z1, l1 );
	//lower block
	z1= z-1;
	addr= BlockAddr(  x & ( H_CHUNK_WIDTH - 1 ), y & ( H_CHUNK_WIDTH - 1 ), z1 );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x, y, z1, l1 );
	//forward block
	y1= ClampY(y + 1);
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x, y1, z, l1 );
	//back block
	y1= ClampY( y - 1 );
	ch= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x, y1, z, l1 );
	//forwad right
	x1= ClampX( x+1 );
	y1= ClampY( y +((x+1)&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x1, y1, z, l1 );
	//back right
	x1= ClampX( x+1 );
	y1= ClampY( y -(x&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x1, y1, z, l1 );
	//forwad left
	x1= ClampX( x-1 );
	y1= ClampY( y +((x+1)&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1  )
		AddFireLightSafe_r( x1, y1, z, l1 );
	//back left
	x1= ClampX( x-1 );
	y1= ClampY( y -(x&1) );
	ch= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 );
	addr= BlockAddr( x1 & ( H_CHUNK_WIDTH - 1 ), y1 & ( H_CHUNK_WIDTH - 1 ), z );
	if( ch->fire_light_map_[addr] < l1 )
		AddFireLightSafe_r( x1, y1, z, l1 );
}

void h_World::ShineFireLight( int x_min, int y_min, int z_min, int x_max, int y_max, int z_max )
{
	(void)z_min;
	(void)z_max;

	x_min>>= H_CHUNK_WIDTH_LOG2;
	x_max>>= H_CHUNK_WIDTH_LOG2;
	y_min>>= H_CHUNK_WIDTH_LOG2;
	y_max>>= H_CHUNK_WIDTH_LOG2;

	int i, j;
	for( i= x_min; i<= x_max; i++ )
	for( j= y_min; j<= y_max; j++ )
	{
		int X, Y;
		X= i<< H_CHUNK_WIDTH_LOG2;
		Y= j<< H_CHUNK_WIDTH_LOG2;

		const std::vector< h_LightSource* >& light_sources= GetChunk( i, j )->GetLightSourceList();
		for( const h_LightSource* source : light_sources )
			AddFireLight_r( source->x_ + X, source->y_ + Y, source->z_, source->LightLevel() );
	}
}

void h_World::AddLightToBorderChunk( unsigned int X, unsigned int Y )
{
	h_Chunk* ch= GetChunk( X, Y );

	int x= X << H_CHUNK_WIDTH_LOG2;
	int y= Y << H_CHUNK_WIDTH_LOG2;

	for( int i= 0; i< H_CHUNK_WIDTH; i++ )
	for( int j= 0; j< H_CHUNK_WIDTH; j++ )
	for( int k= 1; k< H_CHUNK_HEIGHT-1; k++ )
		AddSunLightSafe_r( x+i, y+j, k, ch->SunLightLevel( i, j, k ) );

	//add fire lights to border chunk
	const std::vector< h_LightSource* >& light_sources= ch->GetLightSourceList();
	for( const h_LightSource* source : light_sources )
		AddFireLightSafe_r( source->x_ + x, source->y_ + y, source->z_, source->LightLevel() );
}

void h_World::RelightWaterModifedChunksLight()
{
	const unsigned int c_inv_desiret_chunk_update_chance = 2;

	unsigned int chunk_count= 0;
	h_Chunk* ch;

	for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
	for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
	{
		ch= GetChunk( i, j );
		if( ch->need_update_light_ )
			chunk_count++;
	}

	for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
	for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
	{
		ch= GetChunk( i, j );
		if( ch->need_update_light_ )
		{
			// Chance of one chunk water updating per one phys tick.
			if( phys_processes_rand_.Rand() <=
				phys_processes_rand_.max_rand / ( chunk_count * c_inv_desiret_chunk_update_chance ) )
			{
				int X= i<<H_CHUNK_WIDTH_LOG2;
				int Y= j<<H_CHUNK_WIDTH_LOG2;
				ch->SunRelight();//zero light in chunk and add vertical sun light
				for( int x= 0; x< H_CHUNK_WIDTH; x++ )
				for( int y=0; y< H_CHUNK_WIDTH; y++ )
				for( int z= 1; z< H_CHUNK_HEIGHT-1; z++ )
					AddSunLight_r( X+x, Y+y, z, SunLightLevel( X+x, Y+y, z) );

				for( int x= 0; x< H_CHUNK_WIDTH; x++ )
				for( int z= 1; z< H_CHUNK_HEIGHT-1; z++ )
				{
					AddSunLight_r( X+x, Y-1, z, SunLightLevel( X+x, Y-1, z) );
					AddSunLight_r( X+x, Y+H_CHUNK_WIDTH, z, SunLightLevel( X+x, Y+H_CHUNK_WIDTH, z) );
				}

				for( int y= 0; y< H_CHUNK_WIDTH; y++ )
				for( int z= 1; z< H_CHUNK_HEIGHT-1; z++ )
				{
					AddSunLight_r( X-1, Y+y, z, SunLightLevel( X-1, Y+y, z) );
					AddSunLight_r( X+H_CHUNK_WIDTH, Y+y, z, SunLightLevel( X+H_CHUNK_WIDTH, Y+y, z) );
				}
				ch->need_update_light_= false;

				renderer_->UpdateChunk( i, j );
				renderer_->UpdateChunk( i+1, j+1 );
				renderer_->UpdateChunk( i+1, j-1 );
				renderer_->UpdateChunk( i-1, j+1 );
				renderer_->UpdateChunk( i-1, j-1 );
				renderer_->UpdateChunkWater( i, j );
			}//if rand
		}//if need update light
	} // for ij
}
