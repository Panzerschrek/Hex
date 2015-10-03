#include <cmath>
#include <limits>
#include <vector>

#include <QImage>

#include "../hex.hpp"
#include "../math_lib/assert.hpp"
#include "../math_lib/m_math.h"
#include "../math_lib/rand.h"

#include "world_generator.hpp"

static const float c_pi= 3.1415926535f;
static const float c_2pi= 2.0f * c_pi;

// fixed8_t
static const int c_world_x_scaler= int(H_SPACE_SCALE_VECTOR_X * 256.0f);

// returns value in range [0; 65536)
inline static int Noise2( int x, int y )
{
	int n = x + y * 57;
	n = (n << 13) ^ n;

	return ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff ) >> 15;
}

// returns value in range [0; 65536)
inline static int InterpolatedNoise( int x, int y, int shift )
{
	int X= x>>shift, Y= y>>shift;
	int shift_pow2= 1 << shift;
	int mask= shift_pow2 - 1;

	int dx= x & mask, dy= y & mask;
	int dy1= shift_pow2 - dy;

	int noise[]=
	{
		Noise2(X    , Y    ),
		Noise2(X + 1, Y    ),
		Noise2(X + 1, Y + 1),
		Noise2(X    , Y + 1)
	};

	int interp_x[]=
	{
		noise[3] * dy + noise[0] * dy1,
		noise[2] * dy + noise[1] * dy1
	};

	return ( interp_x[1] * dx + interp_x[0] * (shift_pow2 - dx) ) >> (shift + shift);
}

// returns value in range [0; 65536 + 65536/2 + 65536/4 + ... + 65536/(2^(octaves - 1)) )
static int OctaveNoise( int x, int y, int octaves )
{
	int r= 0;
	for( int i= 0; i < octaves; i++ )
		r += InterpolatedNoise( x, y, octaves - i - 1 ) >> i;

	return r;
}

static void PoissonDiskPoints(
	const unsigned int* size,
	unsigned char* out_data,
	unsigned int min_distanse_div_sqrt2)
{
	m_Rand randomizer;

	const int neighbor_k= 20;
	const float min_dst= float(min_distanse_div_sqrt2) * std::sqrt(2.0f) + 0.01f;
	const float min_dst2= min_dst * min_dst;

	unsigned int grid_size[2];
	for( unsigned int i= 0; i< 2; i++ )
	{
		grid_size[i]= size[i] / min_distanse_div_sqrt2;
		if( grid_size[i] * min_distanse_div_sqrt2 < size[i] ) grid_size[i]++;
	}

	unsigned int max_point_count= grid_size[0] * grid_size[1];
	std::vector<int> grid( max_point_count * 2, -1 );

	std::vector<int*> processing_stack;
	processing_stack.reserve(max_point_count);

	int init_pos[2];
	init_pos[0]= randomizer.RandI( size[0] );
	init_pos[1]= randomizer.RandI( size[1] );

	int init_pos_grid_pos[2];
	init_pos_grid_pos[0]= init_pos[0] / min_distanse_div_sqrt2;
	init_pos_grid_pos[1]= init_pos[1] / min_distanse_div_sqrt2;

	processing_stack.push_back( &grid[ (init_pos_grid_pos[0] + init_pos_grid_pos[1] * grid_size[0]) * 2 ] );
	processing_stack[0][0]= init_pos[0];
	processing_stack[0][1]= init_pos[1];

	while( !processing_stack.empty() )
	{
		int* current_point= processing_stack.back();
		processing_stack.pop_back();

		for( int n= 0; n< neighbor_k; n++ )
		{
			int* cell;
			int pos[2];
			int grid_pos[2];

			float angle= randomizer.RandF( c_pi * 2.0f );
			float r= randomizer.RandF( min_dst, min_dst * 2.0f );

			pos[0]= current_point[0] + int(r * std::cos(angle));
			pos[1]= current_point[1] + int(r * std::sin(angle));
			if( pos[0] < 0 || pos[0] >= int(size[0]) ||
				pos[1] < 0 || pos[1] >= int(size[1]) )
				continue;
			grid_pos[0]= pos[0] / min_distanse_div_sqrt2;
			grid_pos[1]= pos[1] / min_distanse_div_sqrt2;

			for( int y= std::max( grid_pos[1] - 4, 0 ); y< std::min( grid_pos[1] + 4, int(grid_size[1]) ); y++ )
			for( int x= std::max( grid_pos[0] - 4, 0 ); x< std::min( grid_pos[0] + 4, int(grid_size[1]) ); x++ )
			{
				int* cell= &grid[ (x + y * grid_size[0]) * 2 ];
				if( cell[0] != -1 )
				{
					int d_dst[2];
					d_dst[0]= pos[0] - cell[0];
					d_dst[1]= pos[1] - cell[1];

					if( float( d_dst[0] * d_dst[0] + d_dst[1] * d_dst[1] ) < min_dst2 )
						goto xy_loop_break;
				}
			} // search points in nearby grid cells

			cell= &grid[ (grid_pos[0] + grid_pos[1] * grid_size[0]) * 2 ];
			H_ASSERT( cell[0] == -1 );
			cell[0]= pos[0];
			cell[1]= pos[1];
			processing_stack.push_back( cell );

			xy_loop_break:;
		} // try place points near
	} // for points in stack

	float intencity_multipler= 255.0f / float(min_distanse_div_sqrt2 * 3);

	// Draw Voronoy diagramm.
	unsigned char* d= out_data;
	for( int y= 0; y< int(size[1]); y++ )
	{
		int grid_pos[2];
		grid_pos[1]= y / min_distanse_div_sqrt2;
		for( int x= 0; x< int(size[0]); x++, d+= 4 )
		{
			grid_pos[0]= x / min_distanse_div_sqrt2;

			int nearest_point_dst2[2]= { 0xfffffff, 0xfffffff };

			for( int v= std::max( grid_pos[1] - 3, 0 ); v< std::min( grid_pos[1] + 4, int(grid_size[1]) ); v++ )
			for( int u= std::max( grid_pos[0] - 3, 0 ); u < std::min( grid_pos[0] + 4, int(grid_size[0]) ); u++ )
			{
				int* cell= &grid[ (u + v * grid_size[0]) * 2 ];
				if( cell[0] != -1 )
				{
					int d_dst[2];
					d_dst[0]= cell[0] - x;
					d_dst[1]= cell[1] - y;

					int dst2= d_dst[0] * d_dst[0] + d_dst[1] * d_dst[1];
					if( dst2 < nearest_point_dst2[0] )
					{
						nearest_point_dst2[1]= nearest_point_dst2[0];
						nearest_point_dst2[0]= dst2;
					}
					else if( dst2 < nearest_point_dst2[1] )
						nearest_point_dst2[1]= dst2;
				}
			} // for grid uv

			// borders - like points
			{
				int dst2;
				dst2= x * x;
				if( dst2 < nearest_point_dst2[0] ) nearest_point_dst2[0]= dst2;
				dst2= y * y;
				if( dst2 < nearest_point_dst2[0] ) nearest_point_dst2[0]= dst2;
				dst2= int(size[0]) - x; dst2= dst2 * dst2;
				if( dst2 < nearest_point_dst2[0] ) nearest_point_dst2[0]= dst2;
				dst2= int(size[1]) - y; dst2= dst2 * dst2;
				if( dst2 < nearest_point_dst2[0] ) nearest_point_dst2[0]= dst2;
			}

			d[0]= (unsigned char)( std::sqrt(float(nearest_point_dst2[0])) * intencity_multipler );
			d[1]= d[2]= d[3]= 0;

			//d[1]= (unsigned char)( ( std::sqrt(float(nearest_point_dst2[1])) - std::sqrt(float(nearest_point_dst2[0])) ) * intencity_multipler );
			//d[2]= (unsigned char)( std::sqrt(float(nearest_point_dst2[1])) * intencity_multipler );

		} // fot x
	} // for y
}

static void DrawLine(
	int x0, int y0, int x1, int y1,
	unsigned char* framebuffer, unsigned int framebuffer_width )
{
	int dx= x1 - x0;
	int dy= y1 - y0;

	if( std::abs(dx) > std::abs(dy) )
	{
		if( dx < 0 )
		{
			std::swap( x0, x1 );
			std::swap( y0, y1 );
		}
		else if( dx == 0 ) return;

		int y_step_f= (dy<<16) / dx;
		int y_f= y0 << 16;
		for( int x= x0; x < x1; x++, y_f+= y_step_f )
			framebuffer[ x + (y_f >> 16) * framebuffer_width ]= 255;
	}
	else
	{
		if( dy < 0 )
		{
			std::swap( x0, x1 );
			std::swap( y0, y1 );
		}
		else if( dy == 0 ) return;

		int x_step_f= (dx<<16) / dy;
		int x_f= x0 << 16;
		for( int y= y0; y < y1; y++, x_f+= x_step_f )
			framebuffer[ (x_f >> 16) + y * framebuffer_width ]= 255;
	}
}

// Size must be power of 2
static void GenBitmapLod(
	const unsigned int* size, // input size
	const unsigned char* in_data,
	unsigned char* out_data )
{
	unsigned char* dst= out_data;

	for( unsigned int y= 0; y < size[1] / 2; y++ )
	{
		const unsigned char* src0= in_data + y * 2 * size[0];
		const unsigned char* src1= src0 + size[0];
		for( unsigned int x= 0; x < size[0] / 2; x++, src0+=2, src1+=2, dst++ )
		{
			if( src0[0] || src0[1] || src1[0] || src1[1])
				*dst= 255;
			else *dst= 0;
		}
	}
}

// Fast approach to calculate distance field.
// Reject empty pixels use LODs and reject pixel areas, which is too far.
static void FindMinDist_r(
	const unsigned int* size,
	const unsigned int cur_x, const unsigned int cur_y,
	const unsigned int r_x, const unsigned int r_y,
	const unsigned int lod,
	const std::vector< std::vector<unsigned char> >& lods,
	int& nearest_square_dist )
{
	if( lod == 0 )
	{
		if( lods[0][ r_x + r_y * size[0] ] )
		{
			int dx= int(cur_x) - int(r_x);
			int dy= int(cur_y) - int(r_y);
			int dist2= dx * dx + dy * dy;
			if( dist2 < nearest_square_dist )
				nearest_square_dist= dist2;
		}
	}
	else
	{
		if( lods[lod][ r_x + r_y * (size[0] >> lod) ] )
		{
			int dx = std::max( std::abs( int(r_x << lod) - int(cur_x) ) - (1<<lod), 0 );
			int dy = std::max( std::abs( int(r_y << lod) - int(cur_y) ) - (1<<lod), 0 );
			int approx_square_distance= dx * dx + dy * dy;

			if( approx_square_distance <= nearest_square_dist )
				FindMinDist_r( size, cur_x, cur_y, r_x * 2    , r_y * 2    , lod - 1, lods, nearest_square_dist );
			if( approx_square_distance <= nearest_square_dist )
				FindMinDist_r( size, cur_x, cur_y, r_x * 2    , r_y * 2 + 1, lod - 1, lods, nearest_square_dist );
			if( approx_square_distance <= nearest_square_dist )
				FindMinDist_r( size, cur_x, cur_y, r_x * 2 + 1, r_y * 2    , lod - 1, lods, nearest_square_dist );
			if( approx_square_distance <= nearest_square_dist )
				FindMinDist_r( size, cur_x, cur_y, r_x * 2 + 1, r_y * 2 + 1, lod - 1, lods, nearest_square_dist );
		}
	}
}

static void GenDistanceFiled(
	const unsigned int* size,
	unsigned char* in_data,
	unsigned char* out_data,
	int radius )
{
	std::vector< std::vector<unsigned char> > in_data_lods;
	{
		unsigned int xy[2] = { size[0], size[1] };
		while(xy[0] > 0 && xy[1] > 0)
		{
			in_data_lods.push_back( std::vector<unsigned char>() );
			xy[0]>>= 1;
			xy[1]>>= 1;
		}

		in_data_lods[0].resize( size[0] * size[1] );
		memcpy( in_data_lods[0].data(), in_data, size[0] * size[1] );

		for( unsigned int lod= 1; lod < in_data_lods.size(); lod++ )
		{
			in_data_lods[lod].resize( (size[0] >> lod) * (size[1] >> lod) );
			xy[0]= size[0] >> (lod-1);
			xy[1]= size[1] >> (lod-1);
			GenBitmapLod( xy, in_data_lods[lod-1].data(), in_data_lods[lod].data() );
		}
	}

	for( unsigned int y= 0; y < size[1]; y++ )
	for( unsigned int x= 0; x < size[0]; x++ )
	{
		int min_square_distance= radius * radius;

		FindMinDist_r(
			size,
			x, y,
			0, 0,
			in_data_lods.size() - 1, in_data_lods,
			min_square_distance );

		out_data[ x + y * size[0] ]=
			std::min(
				int( std::sqrt(float(min_square_distance)) ) * 255 / radius,
				255 );
	}
}

static void GenHeightmap(
	const unsigned int* size,
	unsigned char* out_data,
	unsigned int seed )
{
	m_Rand randomizer(seed);

	std::vector<unsigned char> lines_data( size[0] * size[1], 0 );

	unsigned char* lines_data_ptr= lines_data.data();

	float r= float( std::min( size[0], size[1] ) / 2 - 2 );
	int r2= int(r * r);
	int center[2]=
	{
		int(size[0]) / 2, int(size[1]) / 2
	};

	for( int y= 0; y < int(size[1]); y++ )
	for( int x= 0; x < int(size[0]); x++ )
	{
		int dx= x - center[0];
		int dy= y - center[1];
		if( dx * dx + dy * dy >= r2 )
			lines_data_ptr[ x + y * size[0] ]= 255;
	}

	// draw lines from circle
	float step_radius= float(size[0] + size[1]) * 0.5f * 0.15f;
	for( unsigned int i= 0; i < 64; i++ )
	{
		int x, y;
		float a= randomizer.RandF( c_2pi );
		x = center[0] + int( r * std::cos(a) );
		y = center[1] + int( r * std::sin(a) );

		while(1)
		{
			float r= randomizer.RandF( step_radius );
			float a= randomizer.RandF( 2.0f * 3.1415926535f );
			int new_x= int( float(x) + r * std::cos(a) );
			int new_y= int( float(y) + r * std::sin(a) );
			if( new_x <= 0 || new_x >= int( size[0] - 1 ) ||
				new_y <= 0 || new_y >= int( size[1] - 1 ) )
				break;

			DrawLine( x, y, new_x, new_y, lines_data.data(), size[0] );
			if( (randomizer() & 3 )== 0 ) break;

			x= new_x;
			y= new_y;
		}
	}

	unsigned int c_radius= 128;
	GenDistanceFiled( size, lines_data.data(), out_data, c_radius );
}

static void GenNoise(
	const unsigned int* size,
	unsigned char* out_data )
{
	const unsigned int c_octaves= 7;

	unsigned int mul = 0;
	// calc 256 + 256/2 + 256 / 4
	for( unsigned int i= 0; i < c_octaves; i++ )
		mul+= 1 << ( 8 - i );
	mul = 65536 / mul;

	unsigned char* dst= out_data;
	for( unsigned int y= 0; y < int(size[1]); y++ )
	for( unsigned int x= 0; x < int(size[0]); x++, dst++ )
		*dst= ( OctaveNoise( x, y, c_octaves ) * mul) >> (8 + 8);
}


/*
-------------------g_WorldGenerator---------------
*/

const unsigned char g_WorldGenerator::c_biomes_colors_[ size_t(g_WorldGenerator::Biome::LastBiome) * 4 ]=
{
	 32,  32, 240, 0, // Sea
	128, 128, 240, 0, // Shelf
	200, 200, 100, 0, // Beach
	 32, 240,  32, 0, // Plains
	 64,  48,  20, 0, // Mountains
};

g_WorldGenerator::g_WorldGenerator(const g_WorldGenerationParameters& parameters)
	: parameters_(parameters)
	, primary_heightmap_  ( parameters.size[0] * parameters.size[1] )
	, secondary_heightmap_( primary_heightmap_.size() )
	, biomes_map_         ( primary_heightmap_.size() )
{
}

void g_WorldGenerator::Generate()
{
	BuildPrimaryHeightmap();
	BuildSecondaryHeightmap();
	BuildBiomesMap();
}

void g_WorldGenerator::DumpDebugResult()
{
	QImage img( parameters_.size[0], parameters_.size[1], QImage::Format_RGBX8888 );
	img.fill(Qt::black);

	unsigned char* bits= img.bits();
	for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
	{
		unsigned int biome= (unsigned int)biomes_map_[i];
		bits[i*4  ]= c_biomes_colors_[biome*4+0];
		bits[i*4+1]= c_biomes_colors_[biome*4+1];
		bits[i*4+2]= c_biomes_colors_[biome*4+2];
		bits[i*4+3]= 0;
	}
	img.save( "biomes_map.png" );
}

unsigned char g_WorldGenerator::GetGroundLevel( int x, int y ) const
{
	x= ( x * c_world_x_scaler) >> 8;

	unsigned int noise=
		InterpolatedNoise( x, y, 5 ) +
		InterpolatedNoise( x, y, 4 ) / 2 +
		InterpolatedNoise( x, y, 3 ) / 4;

	x+= int(parameters_.size[0] / 2) << 2;
	y+= int(parameters_.size[1] / 2) << 2;

	return ( HeightmapValueInterpolated( x, y ) + ( (noise * 8) >> 8 ) ) >> 8;
}

unsigned char g_WorldGenerator::GetSeaLevel() const
{
	return secondary_heightmap_sea_level_;
}

void g_WorldGenerator::BuildPrimaryHeightmap()
{
	GenHeightmap( parameters_.size, primary_heightmap_.data(), parameters_.seed );

	std::vector<unsigned char> white_noise( parameters_.size[0] * parameters_.size[1] );
	GenNoise( parameters_.size, white_noise.data() );

	for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
	{
		primary_heightmap_[i]= ( primary_heightmap_[i] * white_noise[i] ) >> 8;
	}
}

void g_WorldGenerator::BuildSecondaryHeightmap()
{
	for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
	{
		if( primary_heightmap_[i] >= primary_heightmap_sea_level_ )
		{
			unsigned int in_range= 255 - primary_heightmap_sea_level_;
			unsigned int out_range= secondary_heightmap_mountain_top_level_ - secondary_heightmap_sea_level_;

			secondary_heightmap_[i]=
				( primary_heightmap_[i] - primary_heightmap_sea_level_ ) *
				out_range /
				in_range +
				secondary_heightmap_sea_level_;
		}
		else
		{
			unsigned int in_range= primary_heightmap_sea_level_ - 0;
			unsigned int out_range= secondary_heightmap_sea_level_ - secondary_heightmap_sea_bottom_level_;

			secondary_heightmap_[i]=
				( primary_heightmap_[i] - 0 ) *
				out_range /
				in_range +
				secondary_heightmap_sea_bottom_level_;
		}
	}
}

void g_WorldGenerator::BuildBiomesMap()
{
	unsigned int size= parameters_.size[0] * parameters_.size[1];


	std::vector<unsigned char> continent_mask_( size );
	std::vector<unsigned char> distance_filed_( size );

	// Build continental mask, build Primary biomes map. Zero - sea.
	for( unsigned int i= 0; i < size; i++ )
	{
		if( secondary_heightmap_[i] >= primary_heightmap_sea_level_ )
		{
			continent_mask_[i]= 255;
			biomes_map_[i]= Biome::Plains;
		}
		else
		{
			continent_mask_[i]= 0;
			biomes_map_[i]= Biome::Sea;
		}
	}

	// Shelfs.
	const unsigned int c_shelf_radius= 6;
	GenDistanceFiled( parameters_.size, continent_mask_.data(), distance_filed_.data(), c_shelf_radius );
	for( unsigned int i= 0; i < size; i++ )
		if( distance_filed_[i] > 0 && distance_filed_[i] < 255 )
			biomes_map_[i]= Biome::ContinentalShelf;

	// Invert continental mask.
	for( unsigned int i= 0; i < size; i++ )
		continent_mask_[i]= ~continent_mask_[i];

	// Beaches.
	const unsigned int c_beach_radius= 3;
	GenDistanceFiled( parameters_.size, continent_mask_.data(), distance_filed_.data(), c_beach_radius );
	for( unsigned int i= 0; i < size; i++ )
		if( distance_filed_[i] > 0 && distance_filed_[i] < 255 )
			biomes_map_[i]= Biome::SeaBeach;

	// Search mountains.
	// TODO - use primary heightmap.
	const unsigned char c_mountains_altitude= 84;
	for( unsigned int i= 0; i < size; i++ )
		if( secondary_heightmap_[i] >= c_mountains_altitude )
			biomes_map_[i]= Biome::Mountains;
}

unsigned int g_WorldGenerator::HeightmapValueInterpolated( int x, int y ) const
{
	const int shift= 2;
	const int step= 1 << shift;
	const int mask= step - 1;

	int X= x >> shift;
	int Y= y >> shift;
	int dy= y & mask;
	int dy1= step - dy;
	int dx= x & mask;
	int dx1= step - dx;

	int val[4]=
	{
		secondary_heightmap_[ X +      Y    * int(parameters_.size[0]) ],
		secondary_heightmap_[ X + 1 +  Y    * int(parameters_.size[0]) ],
		secondary_heightmap_[ X + 1 + (Y+1) * int(parameters_.size[0]) ],
		secondary_heightmap_[ X +     (Y+1) * int(parameters_.size[0]) ]
	};

	int interp_x[]=
	{
		val[3] * dy + val[0] * dy1,
		val[2] * dy + val[1] * dy1
	};

	return ( interp_x[1] * dx + interp_x[0] * dx1 ) << ( 8 - (shift + shift) );
}
