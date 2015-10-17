#include <cmath>
#include <limits>
#include <vector>

#include <QImage>

#include "../hex.hpp"
#include "../math_lib/assert.hpp"
#include "../math_lib/m_math.h"
#include "../math_lib/rand.h"

#include "noise.hpp"

#include "world_generator.hpp"

static const float c_pi= 3.1415926535f;
static const float c_2pi= 2.0f * c_pi;

// fixed8_t
static const int c_world_x_scaler= int(H_SPACE_SCALE_VECTOR_X * 256.0f);
static const int c_world_x_inv_scaler= int(256.0f / H_SPACE_SCALE_VECTOR_X);

// returns vector of coordinates. (x + y )
static std::vector<g_TreePlantingPoint> PoissonDiskPoints(
	const unsigned int* size,
	unsigned int min_distanse_div_sqrt2,
	unsigned int seed,
	unsigned int* out_grid_size )
{
	m_Rand randomizer( seed );

	const int neighbor_k= 20;
	const float min_dst= float(min_distanse_div_sqrt2) * std::sqrt(2.0f) + 0.01f;
	const float min_dst2= min_dst * min_dst;

	int size_minus_1[2];
	size_minus_1[0]= size[0] - 1;
	size_minus_1[1]= size[1] - 1;

	int grid_size[2];
	int grid_cell_size_to_tex_size_k[2];
	for( unsigned int i= 0; i< 2; i++ )
	{
		grid_size[i]= size[i] / min_distanse_div_sqrt2;
		if( grid_size[i] * min_distanse_div_sqrt2 < size[i] ) grid_size[i]++;
		grid_cell_size_to_tex_size_k[i]= grid_size[i] * min_distanse_div_sqrt2 - size[i];
	}

	// coord int grid - in pixels
	int max_point_count= grid_size[0] * grid_size[1];
	std::vector<int> grid( max_point_count * 2 );
	for( int i= 0; i< max_point_count; i++ )
		grid[i*2]= grid[i*2+1]= -1;

	std::vector<int*> processing_stack;

	int init_pos[2];
	init_pos[0]= randomizer.RandI( size[0] - 1 );
	init_pos[1]= randomizer.RandI( size[1] - 1 );
	int init_pos_grid_pos[2];
	init_pos_grid_pos[0]= init_pos[0] / min_distanse_div_sqrt2;
	init_pos_grid_pos[1]= init_pos[1] / min_distanse_div_sqrt2;
	processing_stack.resize(1);
	processing_stack[0]= &grid[ (init_pos_grid_pos[0] + init_pos_grid_pos[1] * grid_size[0]) * 2 ];
	processing_stack[0][0]= init_pos[0];
	processing_stack[0][1]= init_pos[1];

	while( !processing_stack.empty() )
	{
		int* current_point= processing_stack.back();
		H_ASSERT( current_point[0] != -1 );
		processing_stack.pop_back();

		for( int n= 0; n< neighbor_k; n++ )
		{
			int pos[2];
			int grid_pos[2];
			int* cell;

			float angle= randomizer.RandF( c_2pi );
			float r= randomizer.RandF( min_dst, min_dst*2.0f );

			pos[0]= current_point[0] + int(r * std::cos(angle));
			pos[1]= current_point[1] + int(r * std::sin(angle));
			pos[0]= (pos[0] + size[0]) & size_minus_1[0];
			pos[1]= (pos[1] + size[1]) & size_minus_1[1];
			grid_pos[0]= pos[0] / min_distanse_div_sqrt2;
			grid_pos[1]= pos[1] / min_distanse_div_sqrt2;

			for( int y= grid_pos[1] - 4; y< grid_pos[1] + 4; y++ )
				for( int x= grid_pos[0] - 4; x< grid_pos[0] + 4; x++ )
				{
					int wrap_xy[2];
					wrap_xy[0]= (x + grid_size[0] ) % grid_size[0];
					wrap_xy[1]= (y + grid_size[1] ) % grid_size[1];

					int* cell= &grid[ (wrap_xy[0] + wrap_xy[1] * grid_size[0]) * 2 ];
					if( cell[0] != -1 )
					{
						int d_dst[2];
						d_dst[0]= pos[0] - cell[0] + ( wrap_xy[0] - x ) * min_distanse_div_sqrt2;
						d_dst[1]= pos[1] - cell[1] + ( wrap_xy[1] - y ) * min_distanse_div_sqrt2;

						if( wrap_xy[0] < x ) d_dst[0]+= grid_cell_size_to_tex_size_k[0];
						else if( wrap_xy[0] > x ) d_dst[0]-= grid_cell_size_to_tex_size_k[0];
						if( wrap_xy[1] < y ) d_dst[1]+= grid_cell_size_to_tex_size_k[1];
						else if( wrap_xy[1] > y ) d_dst[1]-= grid_cell_size_to_tex_size_k[1];

						if( float( d_dst[0] * d_dst[0] + d_dst[1] * d_dst[1] ) < min_dst2 )
							goto xy_loop_break;
					}
				}

			cell= &grid[ (grid_pos[0] + grid_pos[1] * grid_size[0]) * 2 ];
			H_ASSERT( cell[0] == -1 );
			cell[0]= pos[0];
			cell[1]= pos[1];
			H_ASSERT( processing_stack.size() < max_point_count );
			processing_stack.push_back( cell );

			xy_loop_break:;
		} // try place points near
	} // while 1

	out_grid_size[0]= grid_size[0];
	out_grid_size[1]= grid_size[1];

	std::vector<g_TreePlantingPoint> result_points( max_point_count );
	for( int i= 0; i< max_point_count; i++ )
		result_points[i]= g_TreePlantingPoint{ short(grid[i*2]), short(grid[i*2+1]) };

	return result_points;
}

static void DrawLine(
	int x0, int y0, int x1, int y1,
	unsigned char* framebuffer, unsigned int framebuffer_width,
	unsigned char color= 255 )
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
			framebuffer[ x + (y_f >> 16) * framebuffer_width ]= color;
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
			framebuffer[ (x_f >> 16) + y * framebuffer_width ]= color;
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
	for( unsigned int i= 0; i < std::min( size[0], size[1] ) / 16; i++ )
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
	unsigned char* out_data,
	int seed )
{
	const unsigned int c_octaves= 7;

	unsigned int mul = 0;
	// calc 256 + 256/2 + 256 / 4
	for( unsigned int i= 0; i < c_octaves; i++ )
		mul+= 1 << ( 8 - i );
	mul = 65536 / mul;

	unsigned char* dst= out_data;
	for( unsigned int y= 0; y < size[1]; y++ )
	for( unsigned int x= 0; x < size[0]; x++, dst++ )
		*dst= ( g_OctaveNoise( x, y, seed, c_octaves ) * mul) >> (8 + 8);
		 /**dst=
			( TriangularOctaveNoise(
				(x * c_world_x_scaler) >> 8,
				y,
				seed,
				c_octaves ) * mul) >> (8 + 8);*/
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
	 96,  72,  30, 0, // Foothills
	 64,  48,  20, 0, // Mountains
	 64, 128, 255, 0, // River
};

const fixed8_t g_WorldGenerator::c_biomes_noise_amplitude_[ size_t(g_WorldGenerator::Biome::LastBiome) ]=
{
	 4 << 8, // Sea
	 3 << 8, // Shelf
	 2 << 8, // Beach
	 5 << 8, // Plains
	12 << 8, // Foothills
	22 << 8, // Mountains
	 0 << 8, // River
};

g_WorldGenerator::g_WorldGenerator(const g_WorldGenerationParameters& parameters)
	: parameters_(parameters)
	, primary_heightmap_  ( parameters.size[0] * parameters.size[1] )
	, secondary_heightmap_( primary_heightmap_.size() )
	, biomes_map_         ( primary_heightmap_.size() )
	, noise_amplitude_map_( primary_heightmap_.size() )
{
}

void g_WorldGenerator::Generate()
{
	BuildPrimaryHeightmap();
	BuildSecondaryHeightmap();
	BuildBiomesMap();
	GenRivers();
	BuildNoiseAmplitudeMap();
	GenTreePlantingMatrix();
}

void g_WorldGenerator::DumpDebugResult()
{
	{
		QImage img( parameters_.size[0], parameters_.size[1], QImage::Format_RGBX8888 );
		img.fill(Qt::black);

		unsigned char* bits= img.bits();
		for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
		{
			bits[i*4  ]=
			bits[i*4+1]=
			bits[i*4+2]= primary_heightmap_[i] >> 8;
			bits[i*4+3]= 0;
		}
		img.save( QString::fromStdString(parameters_.world_dir + "/primary_heightmap.png") );
	}

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
		img.save( QString::fromStdString(parameters_.world_dir + "/biomes_map.png") );
	}

	{
		QImage img( tree_planting_matrix_.size[0], tree_planting_matrix_.size[1], QImage::Format_RGBX8888 );
		img.fill( Qt::black );
		for( const g_TreePlantingPoint& point : tree_planting_matrix_.points_grid_ )
		{
			if( point.x >= 0 )
				img.setPixel( point.x, point.y, 0xFFFFFFFF );
		}
		img.save( QString::fromStdString(parameters_.world_dir + "/trees_matrix.png") );
	}

	{
		QImage img( 1024, 1024, QImage::Format_RGBX8888 );
		img.fill( Qt::black );

		for( unsigned int y= 0; y < img.height(); y++ )
		for( unsigned int x= 0; x < img.width (); x++ )
		{
			unsigned char noise=
				(g_TriangularInterpolatedNoise( x, y, 0, 6 ) >>  9) +
				(g_TriangularInterpolatedNoise( x, y, 0, 5 ) >> 10) +
				(g_TriangularInterpolatedNoise( x, y, 0, 4 ) >> 11) +
				(g_TriangularInterpolatedNoise( x, y, 0, 3 ) >> 12) +
				(g_TriangularInterpolatedNoise( x, y, 0, 2 ) >> 13) +
				(g_TriangularInterpolatedNoise( x, y, 0, 1 ) >> 14) +
				(g_TriangularInterpolatedNoise( x, y, 0, 0 ) >> 15);
			img.setPixel( x, y, noise | (noise<<8) | (noise<<16) | (noise<<24) );
		}
		img.save( QString::fromStdString(parameters_.world_dir + "/triangle_noise.png") );
	}
}

unsigned char g_WorldGenerator::GetGroundLevel( int x, int y ) const
{
	int x_corrected= ( x * c_world_x_scaler) >> 8;

	// HACK. If not do this, borders, parallel to world X axi is to sharply.
	int y_corrected= y - (x&1);

	int noise=
		(g_TriangularInterpolatedNoise( y_corrected, x, parameters_.seed, 6 )     ) +
		(g_TriangularInterpolatedNoise( y_corrected, x, parameters_.seed, 5 ) >> 1) +
		(g_TriangularInterpolatedNoise( y_corrected, x, parameters_.seed, 4 ) >> 2) +
		(g_TriangularInterpolatedNoise( y_corrected, x, parameters_.seed, 3 ) >> 3);
	constexpr const fixed8_t c_noise_scaler= 65536 / ( (1<<8) + (1<<7) + (1<<6) + (1<<5) );
	noise= m_FixedMul<8>( noise, c_noise_scaler );

	x_corrected+= int(parameters_.size[0] / 2) << (H_CHUNK_WIDTH_LOG2 - parameters_.cell_size_log2);
	y          += int(parameters_.size[1] / 2) << (H_CHUNK_WIDTH_LOG2 - parameters_.cell_size_log2);

	fixed8_t noise_amplitude;
	fixed8_t hightmap_value= HeightmapValueInterpolated( x_corrected, y, noise_amplitude );

	fixed8_t result= ( hightmap_value + ( m_FixedMul<8>( noise_amplitude, noise ) >> 8 ) ) >> 8;
	return std::min( result, H_CHUNK_HEIGHT - 4 );
}

unsigned char g_WorldGenerator::GetSeaLevel() const
{
	return secondary_heightmap_sea_level_ >> 8;
}

void g_WorldGenerator::PlantTreesForChunk( int longitude, int latitude, const PlantTreeCallback& plant_tree_callback ) const
{
	// Coordinates in world homogenous space.
	// TODO - here, maybe, are some problems.
	int world_x= ( (longitude << H_CHUNK_WIDTH_LOG2) * c_world_x_scaler ) >> 8;
	int world_y= latitude << H_CHUNK_WIDTH_LOG2;

	// Coordinates inside matrix.
	int matrix_x= world_x & int(tree_planting_matrix_.size[0] - 1);
	int matrix_y= world_y & int(tree_planting_matrix_.size[1] - 1);

	// World space coordinates of hited matrix.
	int matrix_coord_x= world_x - matrix_x;
	int matrix_coord_y= world_y - matrix_y;

	// Coordinate of cell in matrix.
	int cell_x= matrix_x / int(tree_planting_matrix_.grid_cell_size);
	int cell_y= matrix_y / int(tree_planting_matrix_.grid_cell_size);

	// TODO - select right radius
	int radius= 2 + H_CHUNK_WIDTH / int(tree_planting_matrix_.grid_cell_size);

	int grid_cell_size_to_tex_size_k[2];
	for( int i= 0; i < 2; i++ )
		grid_cell_size_to_tex_size_k[i]=
			int(tree_planting_matrix_.grid_size[i]) *
			int(tree_planting_matrix_.grid_cell_size) -
			int(tree_planting_matrix_.size[i]);

	for( int y= cell_y - radius; y <= cell_y + radius; y++ )
	for( int x= cell_x - radius; x <= cell_x + radius; x++ )
	{
		int wrap_x= (x + 2*int(tree_planting_matrix_.grid_size[0])) % int(tree_planting_matrix_.grid_size[0]);
		int wrap_y= (y + 2*int(tree_planting_matrix_.grid_size[1])) % int(tree_planting_matrix_.grid_size[1]);

		const g_TreePlantingPoint& point=
			tree_planting_matrix_.points_grid_[
				wrap_x + wrap_y * int(tree_planting_matrix_.grid_size[0]) ];

		if( point.x >= 0 )
		{
			int tree_x= point.x + matrix_coord_x + (x - wrap_x) * int(tree_planting_matrix_.grid_cell_size);
			int tree_y= point.y + matrix_coord_y + (y - wrap_y) * int(tree_planting_matrix_.grid_cell_size);

			if( wrap_x > x ) tree_x+= grid_cell_size_to_tex_size_k[0];
			else if( wrap_x < x ) tree_x-= grid_cell_size_to_tex_size_k[0];
			if( wrap_y > y ) tree_y+= grid_cell_size_to_tex_size_k[1];
			else if( wrap_y < y ) tree_y-= grid_cell_size_to_tex_size_k[1];

			// Space correction
			tree_x= (tree_x * c_world_x_inv_scaler) >> 8;

			plant_tree_callback( tree_x, tree_y );
		}
	} // for nearest grid cells
}

void g_WorldGenerator::BuildPrimaryHeightmap()
{
	std::vector<unsigned char> base_heightmap( parameters_.size[0] * parameters_.size[1] );
	std::vector<unsigned char> white_noise   ( parameters_.size[0] * parameters_.size[1] );

	GenHeightmap( parameters_.size, base_heightmap.data(), parameters_.seed );
	GenNoise    ( parameters_.size, white_noise   .data(), parameters_.seed );

	for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
		primary_heightmap_[i]= base_heightmap[i] * white_noise[i];
}

void g_WorldGenerator::BuildSecondaryHeightmap()
{
	const fixed8_t c_range_points[]=
	//  primary      seondary
	{
		0 << 8,
		secondary_heightmap_sea_bottom_level_,

		// down sea bottom
		primary_heightmap_sea_level_ - (2<<8),
		secondary_heightmap_sea_level_ - (5<<8),

		primary_heightmap_sea_level_,
		secondary_heightmap_sea_level_,

		// raise land
		primary_heightmap_sea_level_ + (2<<8),
		secondary_heightmap_sea_level_ + (4<<8),

		primary_heightmap_mountains_bottom_level_,
		secondary_heightmap_mountain_bottom_level_,

		255 << 8,
		secondary_heightmap_mountain_top_level_,
	};
	static const size_t c_range_points_count= sizeof(c_range_points) / (sizeof(c_range_points[0]) * 2);

	for( unsigned int i= 0; i < parameters_.size[0] * parameters_.size[1]; i++ )
	{
		fixed8_t primary_val= fixed8_t(primary_heightmap_[i]);
		fixed8_t result= 0;

		for( unsigned int j= 0; j < c_range_points_count - 1; j++ )
		{
			if( primary_val >= c_range_points[j*2] && primary_val < c_range_points[(j+1)*2] )
			{
				fixed8_t  in_range= c_range_points[(j+1)*2  ] - c_range_points[j*2  ];
				fixed8_t out_range= c_range_points[(j+1)*2+1] - c_range_points[j*2+1];
				result=
					(primary_val - c_range_points[j*2]) *
					out_range /
					in_range +
					c_range_points[j*2+1];
				break;
			}
		}
		secondary_heightmap_[i]= result;
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
		if( fixed8_t(primary_heightmap_[i]) >= primary_heightmap_sea_level_ )
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
	const unsigned int c_beach_radius= 2;
	GenDistanceFiled( parameters_.size, continent_mask_.data(), distance_filed_.data(), c_beach_radius );
	for( unsigned int i= 0; i < size; i++ )
		if( distance_filed_[i] > 0 && distance_filed_[i] < 255 )
			biomes_map_[i]= Biome::SeaBeach;

	// Search mountains.
	for( unsigned int i= 0; i < size; i++ )
		if( primary_heightmap_[i] >= primary_heightmap_mountains_bottom_level_ )
			biomes_map_[i]= Biome::Mountains;

	// Foothills.
	for( unsigned int i= 0; i < size; i++ )
		continent_mask_[i]= biomes_map_[i] == Biome::Mountains ? 255 : 0;

	const unsigned int c_foothill_radius= 5;
	GenDistanceFiled( parameters_.size, continent_mask_.data(), distance_filed_.data(), c_foothill_radius );
	for( unsigned int i= 0; i < size; i++ )
		if( distance_filed_[i] > 0 && distance_filed_[i] < 255 )
			biomes_map_[i]= Biome::Foothills;
}

void g_WorldGenerator::GenRivers()
{
	m_Rand randomizer(parameters_.seed);

	unsigned int c_rivers_count= 384;

	int size_x= int(parameters_.size[0]);

	for( unsigned int r= 0; r < c_rivers_count; r++ )
	{
		River river;
		river.emplace_back();

		river.back().x= randomizer.RandI( int(parameters_.size[0]) >> 3, int(parameters_.size[0] * 7) >> 3 ) << 8;
		river.back().y= randomizer.RandI( int(parameters_.size[1]) >> 3, int(parameters_.size[1] * 7) >> 3 ) << 8;

		unsigned int iter= 0;
		while(iter < 1024)
		{
			int x= river.back().x >> 8;
			int y= river.back().y >> 8;

			Biome& biome= biomes_map_[ x + y * size_x ];
			if( biome == Biome::Sea || biome == Biome::ContinentalShelf || biome == Biome::River )
				break;
			biome= river.size() == 1 ? Biome::Mountains : Biome::River;

			int c_radius[]= { /*1, 2,*/3, 4, 7, 11, 15, 20, 26 };
			int min_point[2]= { x, y };
			fixed8_t base_altitude= primary_heightmap_[ x + y * size_x ];
			fixed8_t min_altitude= base_altitude;
			for( int r : c_radius )
			{
				int r2= r * r + 2;
				for( int yy= y - r; yy <= y + r; yy++ )
				{
					int yy2= (yy - y) * (yy - y);
					for( int xx= x - r; xx <= x + r; xx++ )
					{
						int xx2= (xx - x) * (xx - x);
						if( yy2 + xx2 > r2 ) continue;

						fixed8_t altitude= primary_heightmap_[ xx + yy * size_x ];
						if( altitude < min_altitude )
						{
							min_altitude= altitude;
							min_point[0]= xx;
							min_point[1]= yy;
						}
					}
				}
				if( min_altitude < base_altitude ) break;
			}

			RiverPoint new_point;
			new_point.x= min_point[0] << 8;
			new_point.y= min_point[1] << 8;
			river.push_back(new_point);

			/*fixed8_t gradient[2];
			gradient[0]= primary_heightmap_[ x + y * size_x ] - primary_heightmap_[ x+4 +  y    * size_x ];
			gradient[1]= primary_heightmap_[ x + y * size_x ] - primary_heightmap_[ x   + (y+4) * size_x ];
			if( gradient[0] == 0 && gradient[1] == 0 ) break;

			float gradient_inv_length= float(1<<8) / std::sqrt( float(gradient[0] * gradient[0] + gradient[1] * gradient[1]) );
			gradient[0]= fixed8_t( float(gradient[0]) * gradient_inv_length );
			gradient[1]= fixed8_t( float(gradient[1]) * gradient_inv_length );

			fixed8_t step= randomizer.RandI( 1 << 8, 2 << 8 );

			RiverPoint new_point;
			new_point.x= river.back().x + m_FixedMul<8>( step, gradient[0] );
			new_point.y= river.back().y + m_FixedMul<8>( step, gradient[1] );
			river.push_back(new_point);*/

			iter++;
		}

		for( unsigned int i= 0; i < river.size() - 1; i++ )
		{
			DrawLine(
				river[i  ].x >> 8, river[i  ].y >> 8,
				river[i+1].x >> 8, river[i+1].y >> 8,
				(unsigned char*)biomes_map_.data(), size_x,
				(unsigned char)Biome::River );
		}
	}
}

void g_WorldGenerator::BuildNoiseAmplitudeMap()
{
	const int c_kernel_radius= 2;
	const int c_kernel_area= (c_kernel_radius+1) * (c_kernel_radius+1);

	for( unsigned int y= 0; y < parameters_.size[1]; y++ )
	{
		// Set Y borders amplitude
		if( y < c_kernel_radius || y >= parameters_.size[0] - c_kernel_radius )
		{
			for( unsigned int x= 0; x < parameters_.size[0]; x++ )
				noise_amplitude_map_[ x + y * parameters_.size[0] ]=
					c_biomes_noise_amplitude_[ (int)biomes_map_[x + y * parameters_.size[0]] ];
			continue;
		}

		// Set X borders amplitude
		for( unsigned int x= 0; x < c_kernel_radius; x++ )
			noise_amplitude_map_[ x + y * parameters_.size[0] ]=
				c_biomes_noise_amplitude_[ (int)biomes_map_[x + y * parameters_.size[0]] ];
		for( unsigned int x= parameters_.size[0] - c_kernel_radius; x < parameters_.size[0]; x++ )
			noise_amplitude_map_[ x + y * parameters_.size[0] ]=
				c_biomes_noise_amplitude_[ (int)biomes_map_[x + y * parameters_.size[0]] ];

		// Set main area amplitude.
		for( unsigned int x= c_kernel_radius; x < parameters_.size[0] - c_kernel_radius; x++ )
		{
			fixed8_t avg_amplitude= 0;
			for( int j= -c_kernel_radius; j <= c_kernel_radius; j++ )
			for( int i= -c_kernel_radius; i <= c_kernel_radius; i++ )
				avg_amplitude+=
					c_biomes_noise_amplitude_[ (int)biomes_map_[ int(x+i) + int(y+j) * int(parameters_.size[0]) ] ];
			noise_amplitude_map_[ x + y * parameters_.size[0] ]= avg_amplitude / c_kernel_area;
		}
	}
}

void g_WorldGenerator::GenTreePlantingMatrix()
{
	H_ASSERT(tree_planting_matrix_.points_grid_.empty());

	// Repeat planting matrix each 512 meters.
	tree_planting_matrix_.size[0]=
	tree_planting_matrix_.size[1]= 512;

	tree_planting_matrix_.grid_cell_size= 5;

	tree_planting_matrix_.points_grid_=
		PoissonDiskPoints(
			tree_planting_matrix_.size,
			tree_planting_matrix_.grid_cell_size,
			parameters_.seed,
			tree_planting_matrix_.grid_size );
}

fixed8_t g_WorldGenerator::HeightmapValueInterpolated( int x, int y, fixed8_t& out_noise_amplitude ) const
{
	const int shift= H_CHUNK_WIDTH_LOG2 - parameters_.cell_size_log2;
	const int step= 1 << shift;
	const int mask= step - 1;

	int X= x >> shift;
	int Y= y >> shift;
	int dy= y & mask;
	int dy1= step - dy;
	int dx= x & mask;
	int dx1= step - dx;

	H_ASSERT( X >= 0 && X < int(parameters_.size[0] - 1) );
	H_ASSERT( Y >= 0 && Y < int(parameters_.size[1] - 1) );

	int size_x= int(parameters_.size[0]);

	fixed8_t noise_amplitude_val[4]=
	{
		noise_amplitude_map_[ X +      Y    * size_x ],
		noise_amplitude_map_[ X + 1 +  Y    * size_x ],
		noise_amplitude_map_[ X + 1 + (Y+1) * size_x ],
		noise_amplitude_map_[ X +     (Y+1) * size_x ]
	};

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

	fixed8_t noise_amplitude_interp_x[]=
	{
		noise_amplitude_val[3] * dy + noise_amplitude_val[0] * dy1,
		noise_amplitude_val[2] * dy + noise_amplitude_val[1] * dy1
	};

	out_noise_amplitude= ( noise_amplitude_interp_x[1] * dx + noise_amplitude_interp_x[0] * dx1 ) >> (shift + shift);

	return ( interp_x[1] * dx + interp_x[0] * dx1 ) >> (shift + shift);
}
