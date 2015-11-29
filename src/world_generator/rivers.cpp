#include <limits>
#include <cmath>

#include "../math_lib/rand.h"

#include "world_generator.hpp"

#include "rivers.hpp"

// Twodimentional matrix, with points indeces in each matrix cell.
typedef std::vector< std::vector<unsigned short> > g_RiverPointsMatrix;

template< int base >
struct m_FixedVec2
{
	fixed_base_t x, y;
};

template<int base>
static fixed_base_t mFixedVec2Dot( const m_FixedVec2<base>& v0, const m_FixedVec2<base>& v1 )
{
	return m_FixedMul<base>( v0.x, v1.x ) + m_FixedMul<base>( v0.y, v1.y );
}

template<int base>
static bool LinesIntersection(
	const m_FixedVec2<base>& line0v0, const m_FixedVec2<base>& line0v1,
	const m_FixedVec2<base>& line1v0, const m_FixedVec2<base>& line1v1,
	m_FixedVec2<base>& result )
{
	typedef m_FixedVec2<base> VecType;

	VecType l0_vec{ line0v1.x - line0v0.x, line0v1.y - line0v0.y };
	VecType l1_vec{ line1v1.x - line1v0.x, line1v1.y - line1v0.y };
	VecType n0{ -l0_vec.y, l0_vec.x };
	VecType n1{ -l1_vec.y, l1_vec.x };
	fixed_base_t k0= mFixedVec2Dot<base>( n0, line0v0 );
	fixed_base_t k1= mFixedVec2Dot<base>( n1, line1v0 );

	fixed_base_t d= m_FixedMul<base>( n0.x, n1.y ) - m_FixedMul<8>( n0.y, n1.x );
	if( d == 0 ) return false;

	result.x= m_FixedMul<base>( k0, n1.y ) - m_FixedMul<8>( k1, n0.y );
	result.y= m_FixedMul<base>( k1, n0.x ) - m_FixedMul<8>( k0, n1.x );
	result.x= m_FixedDiv<base>( result.x, d );
	result.y= m_FixedDiv<base>( result.y, d );

	VecType check_vec0{ result.x - line0v0.x, result.y - line0v0.y };
	VecType check_vec1{ result.x - line1v0.x, result.y - line1v0.y };
	fixed_base_t l0_vec_squre_len= mFixedVec2Dot<base>( l0_vec, l0_vec );
	fixed_base_t l1_vec_squre_len= mFixedVec2Dot<base>( l1_vec, l1_vec );
	fixed_base_t proj0= mFixedVec2Dot<8>( check_vec0, l0_vec );
	fixed_base_t proj1= mFixedVec2Dot<8>( check_vec1, l1_vec );

	return
		proj0 >= 0 && proj0 < l0_vec_squre_len &&
		proj1 >= 0 && proj1 < l1_vec_squre_len;
}

template<int base>
static void FindNearestPointOnEdge(
	const m_FixedVec2<base>& edgev0, const m_FixedVec2<base>& edgev1,
	const m_FixedVec2<base>& point,
	m_FixedVec2<base>& result_point, fixed_base_t& out_square_distance )
{
	m_FixedVec2<base> edge_vec{ edgev1.x - edgev0.x, edgev1.y - edgev0.y };
	m_FixedVec2<base> vec_to_edge{ point.x - edgev0.x, point.y - edgev0.y };

	fixed_base_t line_square_length= mFixedVec2Dot<base>( edge_vec, edge_vec );
	fixed_base_t dot= mFixedVec2Dot<base>( vec_to_edge, edge_vec );

	if( line_square_length == 0 || dot <= 0 )
	{
		result_point= edgev0;
		out_square_distance= mFixedVec2Dot<base>( vec_to_edge, vec_to_edge );
	}
	else if( dot >= line_square_length )
	{
		result_point= edgev1;
		m_FixedVec2<base> vec_to_v1{ point.x - edgev1.x, point.y - edgev1.y };
		out_square_distance= mFixedVec2Dot<base>( vec_to_v1, vec_to_v1 );
	}
	else
	{
		fixed_base_t div= m_FixedDiv<base>( dot, line_square_length );
		result_point.x= edgev0.x + m_FixedMul<base>( edge_vec.x, div );
		result_point.y= edgev0.y + m_FixedMul<base>( edge_vec.y, div );

		m_FixedVec2<base> vec_to_result_point{ point.x - result_point.x, point.y - result_point.y };
		out_square_distance= mFixedVec2Dot<base>( vec_to_result_point, vec_to_result_point );
	}
}

static void SplitRiver(
	g_RiverSystem& river_system,
	g_RiverSegment& src_river,
	g_RiverSegment& inflowing_river,
	const m_FixedVec2<8>& split_pont,
	size_t splitted_segment_number )
{
	H_ASSERT( splitted_segment_number + 1 < src_river.points_indeces.size() );

	unsigned short src_segment_id= static_cast<unsigned short>( &src_river - &river_system.rivers_segments.front() );
	unsigned short new_segment_id= static_cast<unsigned short>( river_system.rivers_segments.size() );
	//river_system.rivers_segments.emplace_back();
	g_RiverSegment new_segment;

	// Put split point into new segment.
	{
		unsigned short new_point_index= river_system.rivers_points.size();
		river_system.rivers_points.push_back({
			split_pont.x, split_pont.y,
			new_segment_id,
			0 });

		new_segment.points_indeces.push_back( new_point_index );
	}

	// Copy points from old river to new segment. Change indecces.
	for( size_t i= splitted_segment_number + 1; i < src_river.points_indeces.size(); i++ )
	{
		g_RiverPoint& point= river_system.rivers_points[ src_river.points_indeces[i] ];
		point.river_id= new_segment_id;
		point.index_in_river_segment= new_segment.points_indeces.size();

		new_segment.points_indeces.push_back( src_river.points_indeces[i] );
	}

	// Setup relationships of new segment.
	new_segment.child_id= src_river.child_id;
	new_segment.parents_id[0]= src_segment_id;
	new_segment.parents_id[1]= &inflowing_river - &river_system.rivers_segments.front();
	inflowing_river.child_id= new_segment_id;

	// Setup relationships of src segment.
	src_river.child_id= new_segment_id;

	{
		// Remove old points.
		src_river.points_indeces.resize( splitted_segment_number + 1 );

		// Put split point into old river segment.
		unsigned short new_point_index= river_system.rivers_points.size();
		river_system.rivers_points.push_back({
			split_pont.x, split_pont.y,
			new_segment_id,
			static_cast<unsigned short>(src_river.points_indeces.size()) });

		src_river.points_indeces.push_back( new_point_index );
	}

	// Put new segment to tontainer.
	river_system.rivers_segments.push_back( std::move(new_segment) );
}

static void MarkShortRivers( g_RiverSystem& river_system )
{
	// TODO - colibrate constants.
	const float c_inflow_min_length= 3.5f;
	const float c_single_river_min_length= 6.0f;

	for( g_RiverSegment& river : river_system.rivers_segments )
	{
		if( river.parents_id[0] == 0 && river.parents_id[1] == 0 )
		{
			float length= 0.0f;
			for( unsigned int i= 1; i < river.points_indeces.size(); i++ )
			{
				g_RiverPoint& point0= river_system.rivers_points[ river.points_indeces[i-1] ];
				g_RiverPoint& point1= river_system.rivers_points[ river.points_indeces[i  ] ];
				fixed8_t dx= point1.x - point0.x;
				fixed8_t dy= point1.y - point0.y;

				//TODO - remove std::sqrt. We need fixed point square root.
				length += std::sqrt( float( m_FixedSquare<8>(dx) + m_FixedSquare<8>(dy) ) ) * ( 1.0f / 256.0f );
			}
			river.too_short= length < ( river.child_id == 0 ? c_single_river_min_length : c_inflow_min_length );
		}
		else river.too_short= false;
	}
}

void g_WorldGenerator::BuildRiverSystem()
{
	m_Rand randomizer( parameters_.seed );

	// Simple structure for searcing of intersected rivers segments.
	const unsigned int c_point_matrix_cell_size_log2= 3;
	unsigned int points_matrix_size[2]=
	{
		parameters_.size[0] >> c_point_matrix_cell_size_log2,
		parameters_.size[1] >> c_point_matrix_cell_size_log2,
	};
	g_RiverPointsMatrix points_matrix( points_matrix_size[0] * points_matrix_size[1] );

	// Dummy segment - always zero
	g_RiverSystem river_system;
	river_system.rivers_segments.push_back( g_RiverSegment() );

	unsigned int rivers_count= ( parameters_.size[0] * parameters_.size[1] ) >> 7;

	int size_x= int(parameters_.size[0]);
	for( unsigned int river_counter= 0; river_counter < rivers_count; river_counter++ )
	{
		river_system.rivers_segments.emplace_back();
		g_RiverSegment& river= river_system.rivers_segments.back();
		river.parents_id[0]= river.parents_id[1]= river.child_id= 0;
		unsigned short river_id= river_system.rivers_segments.size() - 1;

		river_system.rivers_points.emplace_back();
		g_RiverPoint& point= river_system.rivers_points.back();

		point.x= randomizer.RandI( (int(parameters_.size[0]) >> 4) << 8, (int(parameters_.size[0] * 15) >> 4) << 8 );
		point.y= randomizer.RandI( (int(parameters_.size[1]) >> 4) << 8, (int(parameters_.size[1] * 15) >> 4) << 8 );
		point.river_id= river_id;
		point.index_in_river_segment= river.points_indeces.size();
		river.points_indeces.push_back( river_system.rivers_points.size() - 1 );

		unsigned int iter= 0;
		do
		{
			g_RiverPoint prev_point= river_system.rivers_points[ river.points_indeces.back() ];
			int x= prev_point.x >> 8;
			int y= prev_point.y >> 8;

			Biome& biome= biomes_map_[ x + y * size_x ];
			if( biome == Biome::Sea || biome == Biome::ContinentalShelf )
				break;

			int c_radius[]= { /*1, 2,*/3, 4, 7, 11/*, 15, 20, 26 */};
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
			new_point.x= (min_point[0] << 8) + randomizer.RandI( 1 << 8 );
			new_point.y= (min_point[1] << 8) + randomizer.RandI( 1 << 8 );
			new_point.river_id= river_id;
			new_point.index_in_river_segment= river.points_indeces.size();
			river_system.rivers_points.emplace_back( new_point );
			river.points_indeces.push_back( river_system.rivers_points.size() - 1 );
			size_t new_point_number= river_system.rivers_points.size() - 1;

			m_FixedVec2<8> segment[2]
			{
				{ prev_point.x, prev_point.y },
				{ new_point.x, new_point.y },
			};

			// check near cells
			g_RiverSegment* candidate_river= nullptr;
			size_t candidate_river_edge= 0;
			m_FixedVec2<8> candidate_intersection_point{ 0, 0 };
			fixed8_t candidate_square_distance= std::numeric_limits<fixed8_t>::max();

			const int c_intersect_search_radius= 1;
			for( int y= -c_intersect_search_radius; y <= c_intersect_search_radius; y++ )
			{
				int yy= ( min_point[1] >> c_point_matrix_cell_size_log2 ) + y;
				if( yy < 0 || yy >= int(points_matrix_size[1]) ) continue;
				for( int x= -c_intersect_search_radius; x <= c_intersect_search_radius; x++ )
				{
					int xx= ( min_point[0] >> c_point_matrix_cell_size_log2 ) + x;
					if( xx < 0 || xx >= int(points_matrix_size[0]) ) continue;

					// find points in this cell
					const std::vector<unsigned short>& points_ids= points_matrix[ xx + yy * int(points_matrix_size[0]) ];
					for( unsigned short point_id : points_ids )
					{
						// find river segment of point
						g_RiverPoint& try_point= river_system.rivers_points[ point_id ];
						g_RiverSegment& try_river= river_system.rivers_segments[ try_point.river_id ];
						int point_index_in_river= int(try_point.index_in_river_segment);
						for( int p= std::max( 1, point_index_in_river - 3 ); p < std::min( int(try_river.points_indeces.size()), point_index_in_river + 3 ); p++ )
						{
							g_RiverPoint& point0= river_system.rivers_points[ try_river.points_indeces[p-1] ];
							g_RiverPoint& point1= river_system.rivers_points[ try_river.points_indeces[p  ] ];
							m_FixedVec2<8> dst_segment[2]
							{
								{ point0.x, point0.y },
								{ point1.x, point1.y },
							};

							m_FixedVec2<8> intersection_point;
							if( LinesIntersection<8>( segment[0], segment[1], dst_segment[0], dst_segment[1], intersection_point ) )
							{
								m_FixedVec2<8> vec_to_intersection_point;
								vec_to_intersection_point.x= intersection_point.x - prev_point.x;
								vec_to_intersection_point.y= intersection_point.x - prev_point.y;

								fixed8_t square_distance= mFixedVec2Dot<8>( vec_to_intersection_point, vec_to_intersection_point );

								if( square_distance < candidate_square_distance )
								{
									candidate_square_distance= square_distance;
									candidate_river_edge= p - 1;
									candidate_river= &try_river;
									candidate_intersection_point= intersection_point;
								}
							} // if is intersection
							else
							{
								// Disagonal of quad with side size 2.
								const fixed8_t c_min_square_distance= (2 * 2 * 2)<<8;

								m_FixedVec2<8> nearest_point_on_edge;
								fixed8_t square_distance_to_nearest_point_on_edge;

								FindNearestPointOnEdge<8>(
									dst_segment[0], dst_segment[1],
									segment[0],
									nearest_point_on_edge, square_distance_to_nearest_point_on_edge );

								if( square_distance_to_nearest_point_on_edge <= c_min_square_distance &&
									square_distance_to_nearest_point_on_edge < candidate_square_distance )
								{
									candidate_square_distance= square_distance_to_nearest_point_on_edge;
									candidate_river_edge= p - 1;
									candidate_river= &try_river;
									candidate_intersection_point= nearest_point_on_edge;
								}
							} // Check, if point too close to river.
						} // for edges of try river
					}
				} // for x
			} // for y

			if( candidate_river )
			{
				SplitRiver(
					river_system,
					*candidate_river, river,
					candidate_intersection_point, candidate_river_edge );

				river_system.rivers_points[ new_point_number ].x= candidate_intersection_point.x;
				river_system.rivers_points[ new_point_number ].y= candidate_intersection_point.y;

				break; // End river.
			}

		} while( (++iter) < 256 ); // place river segments

		for( unsigned short point_index : river.points_indeces )
		{
			const g_RiverPoint& point= river_system.rivers_points[ point_index ];
			unsigned int x= point.x >> ( 8 + c_point_matrix_cell_size_log2 );
			unsigned int y= point.y >> ( 8 + c_point_matrix_cell_size_log2 );

			points_matrix[ x + y * points_matrix_size[0] ].push_back( point_index );
		}
	} // try place river

	MarkShortRivers( river_system );

	for( const g_RiverSegment& river : river_system.rivers_segments )
		if( !river.too_short )
		{
			for( unsigned int i= 1; i < river.points_indeces.size(); i++ )
			{
				const g_RiverPoint& point0= river_system.rivers_points[ river.points_indeces[i-1] ];
				const g_RiverPoint& point1= river_system.rivers_points[ river.points_indeces[i  ] ];

				extern void DrawLine(
							int x0, int y0, int x1, int y1,
							unsigned char* framebuffer, unsigned int framebuffer_width,
							unsigned char color= 255 );
				DrawLine(
					point0.x >> 8, point0.y >> 8,
					point1.x >> 8, point1.y >> 8,
					(unsigned char*)biomes_map_.data(), size_x,
					(unsigned char)Biome::River );
			}
		}

	river_system_= std::move( river_system );
}
