#include "../math_lib/rand.h"

#include "world_generator.hpp"

#include "rivers.hpp"

// Twodimentional matrix, with points indeces in each matrix cell.
typedef std::vector< std::vector<unsigned short> > g_RiverPointsMatrix;


void DrawLine(
	int x0, int y0, int x1, int y1,
	unsigned char* framebuffer, unsigned int framebuffer_width,
	unsigned char color= 255 );

void g_WorldGenerator::BuildRiverSystem()
{
	m_Rand randomizer( parameters_.seed );

	// Simple structure for searcing of intersected rivers segments.
	const unsigned int c_point_matrix_cell_size_log2= 2;
	unsigned int points_matrix_size[2]=
	{
		parameters_.size[0] >> c_point_matrix_cell_size_log2,
		parameters_.size[1] >> c_point_matrix_cell_size_log2,
	};
	g_RiverPointsMatrix points_matrix( points_matrix_size[0] * points_matrix_size[1] );

	// Dummy segment - always zero
	g_RiverSystem river_system;
	river_system.rivers_segments.push_back( g_RiverSegment() );

	unsigned int c_rivers_count= 384;
	int size_x= int(parameters_.size[0]);
	for( unsigned int r= 0; r < c_rivers_count; r++ )
	{
		river_system.rivers_segments.emplace_back();
		g_RiverSegment& river= river_system.rivers_segments.back();
		unsigned short river_id= river_system.rivers_segments.size() - 1;

		river_system.rivers_points.emplace_back();
		g_RiverPoint& point= river_system.rivers_points.back();

		point.x= randomizer.RandI( int(parameters_.size[0]) >> 3, int(parameters_.size[0] * 7) >> 3 ) << 8;
		point.y= randomizer.RandI( int(parameters_.size[1]) >> 3, int(parameters_.size[1] * 7) >> 3 ) << 8;
		point.river_id= river_id;
		river.points_indeces.push_back( river_system.rivers_points.size() - 1 );

		unsigned int iter= 0;
		do
		{
			int x= river_system.rivers_points[ river.points_indeces.back() ].x >> 8;
			int y= river_system.rivers_points[ river.points_indeces.back() ].y >> 8;

			Biome& biome= biomes_map_[ x + y * size_x ];
			if( biome == Biome::Sea || biome == Biome::ContinentalShelf )
				break;

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

			if( min_altitude >= base_altitude ) break; // river end

			g_RiverPoint new_point;
			new_point.x= min_point[0] << 8;
			new_point.y= min_point[1] << 8;
			new_point.river_id= river_id;
			river_system.rivers_points.emplace_back(new_point);
			river.points_indeces.push_back( river_system.rivers_points.size() - 1 );

		} while( (++iter) < 256 );
	}

	for( const g_RiverSegment& river : river_system.rivers_segments )
		for( unsigned int i= 1; i < river.points_indeces.size(); i++ )
		{
			const g_RiverPoint& point0= river_system.rivers_points[ river.points_indeces[i-1] ];
			const g_RiverPoint& point1= river_system.rivers_points[ river.points_indeces[i  ] ];
			DrawLine(
				point0.x >> 8, point0.y >> 8,
				point1.x >> 8, point1.y >> 8,
				(unsigned char*)biomes_map_.data(), size_x,
				(unsigned char)Biome::River );
		}
}
