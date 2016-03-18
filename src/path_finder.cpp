#include <cstring>
#include <limits>
#include <vector>

#include "math_lib/assert.hpp"
#include "world.hpp"
#include "path_finder.hpp"

struct WavefrontPoint
{
	typedef short CoordinateType;
	static_assert(
		std::numeric_limits<CoordinateType>::max() >
		( (H_MAX_LONGITUDE > H_MAX_LATITUDE ? H_MAX_LONGITUDE : H_MAX_LATITUDE) << H_CHUNK_WIDTH_LOG2 ),
		"too small CoordinateType" );

	unsigned int prev_point_id;
	CoordinateType x, y, z;

	WavefrontPoint(){}
	WavefrontPoint( CoordinateType in_x, CoordinateType in_y, CoordinateType in_z, unsigned int in_prev_point_id )
		: prev_point_id(in_prev_point_id)
		, x(in_x), y(in_y), z(in_z)
	{}
};

const constexpr unsigned int h_PathFinder::c_cube_size_log2_;
const constexpr unsigned int h_PathFinder::c_cube_size_;
const constexpr unsigned int h_PathFinder::c_max_search_distance_;

h_PathFinder::h_PathFinder( const h_World &world )
	: world_(world)
	, path_length_(0)
{
}

h_PathFinder::~h_PathFinder()
{
}

// TODO - optimize
// Replace all std::vector with member arrays
bool h_PathFinder::FindPath(
	int src_x, int src_y, int src_z,
	int target_x, int target_y, int target_z,
	unsigned int max_radius )
{
	ClearVisitedCells();

	cube_position_[0]= src_x - (c_cube_size_>>1);
	cube_position_[1]= src_y - (c_cube_size_>>1);
	cube_position_[2]= src_z - (c_cube_size_>>1);

	unsigned int wave_radius= 0;

	std::vector<WavefrontPoint> visited_points;
	std::vector<unsigned int> prev_wavefront;
	std::vector<unsigned int> new_wavefront;

	SetPointVisited( src_x, src_y, src_z );
	visited_points.emplace_back(); // Dummy
	visited_points.emplace_back( src_x, src_y, src_z, 0 );
	prev_wavefront.emplace_back( visited_points.size() - 1 );

	while( wave_radius < max_radius )
	{
		while( !prev_wavefront.empty() )
		{
			unsigned int prev_point_id= prev_wavefront.back();
			prev_wavefront.pop_back();
			WavefrontPoint wavefront_point= visited_points[ prev_point_id ];

			short forward_side_y= short( wavefront_point.y + ((wavefront_point.x+1) & 1) );
			short back_side_y= short( wavefront_point.y - (wavefront_point.x & 1) );
			WavefrontPoint neighbors[6]=
			{
				{ wavefront_point.x, short(wavefront_point.y + 1), // FORWARD
					wavefront_point.z, 0 },
				{ wavefront_point.x, short(wavefront_point.y - 1), // BACK
					wavefront_point.z, 0 },
				{ short(wavefront_point.x + 1), forward_side_y, // FORWARD_RIGHT
					wavefront_point.z, 0 },
				{ short(wavefront_point.x + 1), back_side_y, // BACK_RIGHT
					wavefront_point.z, 0 },
				{ short(wavefront_point.x - 1), forward_side_y, // FORWARD_LEFT
					wavefront_point.z, 0 },
				{ short(wavefront_point.x - 1), back_side_y, // BACK_LEFT
					wavefront_point.z, 0 },
			};

			const unsigned char* current_point_taransparency_data;
			{
				int world_relative_x= wavefront_point.x - ( world_.Longitude() << H_CHUNK_WIDTH_LOG2 );
				int world_relative_y= wavefront_point.y - ( world_.Latitude () << H_CHUNK_WIDTH_LOG2 );
				int chunk_local_x= wavefront_point.x & (H_CHUNK_WIDTH - 1);
				int chunk_local_y= wavefront_point.y & (H_CHUNK_WIDTH - 1);
				// TODO - maybe just skip points, outside world borders?
				H_ASSERT( world_relative_x >= 0 );
				H_ASSERT( world_relative_y >= 0 );

				const h_Chunk* chunk= world_.GetChunk( world_relative_x >> H_CHUNK_WIDTH_LOG2, world_relative_y >> H_CHUNK_WIDTH_LOG2 );
				current_point_taransparency_data= chunk->GetTransparencyData() + BlockAddr( chunk_local_x, chunk_local_y, 0 );
			}

			for( const WavefrontPoint& neighbor : neighbors )
			{
				if( IsPointVisited( neighbor.x, neighbor.y, neighbor.z ) ) continue;

				int world_relative_x= neighbor.x - ( world_.Longitude() << H_CHUNK_WIDTH_LOG2 );
				int world_relative_y= neighbor.y - ( world_.Latitude () << H_CHUNK_WIDTH_LOG2 );
				int chunk_local_x= neighbor.x & (H_CHUNK_WIDTH - 1);
				int chunk_local_y= neighbor.y & (H_CHUNK_WIDTH - 1);
				// TODO - maybe just skip points, outside world borders?
				H_ASSERT( world_relative_x >= 0 );
				H_ASSERT( world_relative_y >= 0 );

				const h_Chunk* chunk= world_.GetChunk( world_relative_x >> H_CHUNK_WIDTH_LOG2, world_relative_y >> H_CHUNK_WIDTH_LOG2 );
				int address= BlockAddr( chunk_local_x, chunk_local_y, neighbor.z );
				const unsigned char* transparency_data= chunk->GetTransparencyData();

				bool can_step_forward=
					( transparency_data[ address     ] & H_VISIBLY_TRANSPARENCY_BITS ) == TRANSPARENCY_AIR &&
					( transparency_data[ address - 1 ] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_AIR;

				bool can_step_up=
					( transparency_data[ address     ] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_AIR &&
					( transparency_data[ address + 1 ] & H_VISIBLY_TRANSPARENCY_BITS ) == TRANSPARENCY_AIR &&
					( current_point_taransparency_data[ wavefront_point.z + 1 ] & H_VISIBLY_TRANSPARENCY_BITS )== TRANSPARENCY_AIR;
				bool can_step_down=
					neighbor.z >= 2 &&
					( transparency_data[ address     ] & H_VISIBLY_TRANSPARENCY_BITS ) == TRANSPARENCY_AIR &&
					( transparency_data[ address - 1 ] & H_VISIBLY_TRANSPARENCY_BITS ) == TRANSPARENCY_AIR &&
					( transparency_data[ address - 2 ] & H_VISIBLY_TRANSPARENCY_BITS ) != TRANSPARENCY_AIR;

				if( neighbor.x == target_x && neighbor.y == target_y &&
					(neighbor.z == target_z ||
					(neighbor.z - 1 == target_z && can_step_down) ||
					(neighbor.z + 1 == target_z && can_step_up) ) )
				{
					path_[0].x= target_x;
					path_[0].y= target_y;
					path_[0].z= target_z;
					unsigned int i= 1;
					H_ASSERT( wavefront_point.prev_point_id < visited_points.size() );
					const WavefrontPoint* point= &visited_points[ prev_point_id ];
					while( point->prev_point_id != 0 )
					{
						path_[i].x= point->x;
						path_[i].y= point->y;
						path_[i].z= point->z;
						H_ASSERT( point->prev_point_id < visited_points.size() );
						point= &visited_points[ point->prev_point_id ];
						i++;
					}
					path_length_= i;
					H_ASSERT( path_length_ == wave_radius + 1 );
					return true;
				}

				if( can_step_forward )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z, prev_point_id );
					new_wavefront.emplace_back( visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
				}
				// step down
				else if( can_step_down )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z - 1, prev_point_id );
					new_wavefront.emplace_back( visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z - 1 );
				}
				else if( can_step_up )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z + 1, prev_point_id );
					new_wavefront.emplace_back( visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z + 1 );
				}
			} // for neighbors
		} // for current wavefront

		prev_wavefront.swap( new_wavefront );
		wave_radius++;
	} // while max radius not reached

	path_length_= 0;
	return false;
}

const h_PathPoint* h_PathFinder::GetPathPoints() const
{
	return path_;
}

unsigned int h_PathFinder::GetPathLength() const
{
	return path_length_;
}

unsigned int h_PathFinder::GetMaxSearchRadius()
{
	return c_max_search_distance_;
}

void h_PathFinder::ClearVisitedCells()
{
	std::memset( visited_cells_bitmap_, 0, sizeof(visited_cells_bitmap_) );
}

void h_PathFinder::SetPointVisited( int x, int y, int z )
{
	int address= GetBitmapAddress( x, y, z );
	visited_cells_bitmap_[ address >> 3 ]|= 1 << (address & 7);
}

bool h_PathFinder::IsPointVisited( int x, int y, int z ) const
{
	int address= GetBitmapAddress( x, y, z );
	return bool( visited_cells_bitmap_[ address >> 3 ] & ( 1 << (address & 7) ) );
}

int h_PathFinder::GetBitmapAddress( int x, int y, int z ) const
{
	int local_x= x - cube_position_[0];
	int local_y= y - cube_position_[1];
	int local_z= z - cube_position_[2];

	H_ASSERT( local_x >= 0 && local_x < int(c_cube_size_) );
	H_ASSERT( local_y >= 0 && local_y < int(c_cube_size_) );
	H_ASSERT( local_z >= 0 && local_z < int(c_cube_size_) );

	return
		local_x |
		( local_y << c_cube_size_log2_ ) |
		( local_z << (c_cube_size_log2_ << 1) );
}
