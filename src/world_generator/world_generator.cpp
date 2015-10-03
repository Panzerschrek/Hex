#include <cmath>
#include <limits>
#include <vector>

#include <QImage>

#include "../math_lib/assert.hpp"
#include "../math_lib/m_math.h"
#include "../math_lib/rand.h"

#include "world_generator.hpp"

static const float c_pi= 3.1415926535f;
static const float c_2pi= 2.0f * c_pi;

// returns value in range [0; 65536)
inline static int Noise2( int x, int y )
{
	int n = x + y * 57;
	n = (n << 13) ^ n;

	return ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff ) >> 15;
}

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

static int OctaveNoise( int x, int y, int octaves )
{
	int r= 0;
	for( int i= 0; i < octaves; i++ )
		r += InterpolatedNoise( x, y, octaves - i - 1 ) >> i;

	return r >> 1;
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

static void GenDistanceFiled(
	const unsigned int* size,
	unsigned char* in_data,
	unsigned char* out_data )
{
	const int c_radius= 128;

	for( int y= 0; y < int(size[1]); y++ )
	{
		int yy_min= std::max( y - c_radius, 0 );
		int yy_max= std::min( y + c_radius, int(size[1] - 1) );

		for( int x= 0; x < int(size[0]); x++ )
		{
			int xx_min= std::max( x - c_radius, 0 );
			int xx_max= std::min( x + c_radius, int(size[0] - 1) );

			int min_dist2= c_radius * c_radius;
			for( int yy= yy_min; yy <= yy_max; yy++ )
			for( int xx= xx_min; xx <= xx_max; xx++ )
			{
				if( in_data[ xx + yy * int(size[0]) ] )
				{
					int dist2= (x - xx) * (x - xx) + (y - yy) * (y - yy);
					if( dist2 < min_dist2 ) min_dist2= dist2;
				}
			}

			out_data[ x + y * int(size[0]) ]=
				std::min(
					int( std::sqrt(float(min_dist2)) ) * 255 / c_radius,
					255 );
		} // for x
	} // for y
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

	// draw circle
	/*const unsigned int c_circle_segments= 32;
	for( unsigned int i= 0; i < c_circle_segments; i++ )
	{
		float a0= float(i  ) / float(c_circle_segments) * c_2pi;
		float a1= float(i+1) / float(c_circle_segments) * c_2pi;

		int x0= center[0] + int( std::cos(a0) * r );
		int y0= center[1] + int( std::sin(a0) * r );

		int x1= center[0] + int( std::cos(a1) * r );
		int y1= center[1] + int( std::sin(a1) * r );

		DrawLine( x0, y0, x1, y1, lines_data.data(), size[0] );
	}*/

	for( int y= 0; y < int(size[1]); y++ )
	for( int x= 0; x < int(size[0]); x++ )
	{
		int dx= x - center[0];
		int dy= y - center[1];
		if( dx * dx + dy * dy >= r2 )
			lines_data_ptr[ x + y * size[0] ]= 1;
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

	GenDistanceFiled( size, lines_data.data(), out_data );
}


static void GenNoise(
	const unsigned int* size,
	unsigned char* out_data )
{
	unsigned char* dst= out_data;
	for( unsigned int y= 0; y < int(size[1]); y++ )
	for( unsigned int x= 0; x < int(size[0]); x++, dst++ )
		*dst= OctaveNoise( x, y, 7 ) >> 8;
}

g_WorldGenerator::g_WorldGenerator(const g_WorldGenerationParameters& parameters)
	: parameters_(parameters)
{
}

void g_WorldGenerator::Generate()
{
	unsigned int size[2]= { 1024, 1024 };
	std::vector<unsigned char> data( size[0] * size[1] );
	//PoissonDiskPoints( size, data.data(), 73 );

	GenHeightmap( size, data.data(), 42 );

	std::vector<unsigned char> white_noise( size[0] * size[1] );
	GenNoise( size, white_noise.data() );

	for( unsigned int i= 0; i < size[0] * size[1]; i++ )
	{
		data[i]= ( data[i] * white_noise[i] ) >> 8;
		data[i]= data[i] > 44 ? 255 : 0;
	}

	QImage img( size[0], size[1], QImage::Format_RGBX8888 );
	img.fill(Qt::black);

	unsigned char* bits= img.bits();
	for( unsigned int i= 0; i < size[0] * size[1]; i++ )
	{
		bits[i*4  ]=
		bits[i*4+1]=
		bits[i*4+2]= data[i];
		bits[i*4+3]= 0;
	}
	img.save( "heightmap.png" );
}

void g_WorldGenerator::DumpDebugResult()
{
}
