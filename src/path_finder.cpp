#include <cstring>
#include <vector>

#include "math_lib/assert.hpp"
#include "world.hpp"
#include "path_finder.hpp"

struct WavefrontPoint
{
	int x, y, z;
	// In wave this is id of point
	// In list of visitid point this is id of previous point
	unsigned int id;

	WavefrontPoint(){}
	WavefrontPoint( int in_x, int in_y, int in_z, unsigned int in_id )
		: x(in_x), y(in_y), z(in_z)
		, id(in_id)
	{}
};

const constexpr unsigned int h_PathFinder::c_cube_size_log2_;
const constexpr unsigned int h_PathFinder::c_cube_size_;
const constexpr unsigned int h_PathFinder::c_max_search_distance_;

h_PathFinder::h_PathFinder(const h_World &world)
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
	int target_x, int target_y, int target_z )
{
	ClearVisitedCells();

	cube_position_[0]= src_x - (c_cube_size_>>1);
	cube_position_[1]= src_y - (c_cube_size_>>1);
	cube_position_[2]= src_z - (c_cube_size_>>1);

	unsigned int wave_radius= 0;

	std::vector<WavefrontPoint> visited_points;
	std::vector<WavefrontPoint> prev_wavefront;
	std::vector<WavefrontPoint> new_wavefront;

	visited_points.emplace_back(); // Dummy

	SetPointVisited( src_x, src_y, src_z );
	visited_points.emplace_back( src_x, src_y, src_z, 0 );
	prev_wavefront.emplace_back( src_x, src_y, src_z, visited_points.size() - 1 );

	while( wave_radius < c_max_search_distance_ )
	{
		while( !prev_wavefront.empty() )
		{
			WavefrontPoint& wavefront_point= prev_wavefront.back();

			int forward_side_dy= (wavefront_point.x+1) & 1;
			int back_side_dy= wavefront_point.x & 1;
			WavefrontPoint neighbors[6]=
			{
				{ wavefront_point.x, wavefront_point.y + 1, // FORWARD
					wavefront_point.z, 0 },
				{ wavefront_point.x, wavefront_point.y - 1, // BACK
					wavefront_point.z, 0 },
				{ wavefront_point.x + 1, wavefront_point.y + forward_side_dy, // FORWARD_RIGHT
					wavefront_point.z, 0 },
				{ wavefront_point.x + 1, wavefront_point.y - back_side_dy, // BACK_RIGHT
					wavefront_point.z, 0 },
				{ wavefront_point.x - 1, wavefront_point.y + forward_side_dy, // FORWARD_LEFT
					wavefront_point.z, 0 },
				{ wavefront_point.x - 1, wavefront_point.y - back_side_dy, // BACK_LEFT
					wavefront_point.z, 0 },
			};
			for( const WavefrontPoint& neighbor : neighbors )
			{
				if( IsPointVisited( neighbor.x, neighbor.y, neighbor.z ) ) continue;

				int world_relative_x= neighbor.x - ( world_.Longitude() << H_CHUNK_WIDTH_LOG2 );
				int world_relative_y= neighbor.y - ( world_.Latitude () << H_CHUNK_WIDTH_LOG2 );
				int chunk_local_x= neighbor.x & (H_CHUNK_WIDTH - 1);
				int chunk_local_y= neighbor.y & (H_CHUNK_WIDTH - 1);
				H_ASSERT( world_relative_x >= 0 );
				H_ASSERT( world_relative_y >= 0 );

				const h_Chunk* chunk= world_.GetChunk( world_relative_x >> H_CHUNK_WIDTH_LOG2, world_relative_y >> H_CHUNK_WIDTH_LOG2 );
				int address= BlockAddr( chunk_local_x, chunk_local_y, neighbor.z );
				const unsigned char* transparency_data= chunk->GetTransparencyData();

				bool can_step_forward= transparency_data[ address ] == TRANSPARENCY_AIR && transparency_data[ address - 1 ] != TRANSPARENCY_AIR;
				bool can_step_up= transparency_data[ address ] != TRANSPARENCY_AIR && transparency_data[ address + 1 ] == TRANSPARENCY_AIR;
				bool can_step_down=
					neighbor.z >= 2 &&
					transparency_data[ address - 1 ] == TRANSPARENCY_AIR &&
					transparency_data[ address - 2 ] != TRANSPARENCY_AIR;

				if( neighbor.x == target_x && neighbor.y == target_y &&
					(neighbor.z == target_z ||
					(neighbor.z - 1 == target_z && can_step_down) ||
					(neighbor.z + 1 == target_z && can_step_up) ) )
				{
					path_[0].x= target_x;
					path_[0].y= target_y;
					path_[0].z= target_z;
					unsigned int i= 1;
					H_ASSERT( wavefront_point.id < visited_points.size() );
					const WavefrontPoint* point= &visited_points[ wavefront_point.id ];
					while( point->id != 0 )
					{
						path_[i].x= point->x;
						path_[i].y= point->y;
						path_[i].z= point->z;
						H_ASSERT( point->id < visited_points.size() );
						point= &visited_points[ point->id ];
						i++;
					}
					path_length_= i;
					H_ASSERT( path_length_ == wave_radius + 1 );
					return true;
				}

				if( can_step_forward )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z, wavefront_point.id );
					new_wavefront.emplace_back( neighbor.x, neighbor.y, neighbor.z, visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
				}
				// step down
				else if( can_step_down )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z - 1, wavefront_point.id );
					new_wavefront.emplace_back( neighbor.x, neighbor.y, neighbor.z - 1, visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z - 1 );
				}
				else if( can_step_up )
				{
					visited_points.emplace_back( neighbor.x, neighbor.y, neighbor.z + 1, wavefront_point.id );
					new_wavefront.emplace_back( neighbor.x, neighbor.y, neighbor.z + 1, visited_points.size() - 1 );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z );
					SetPointVisited( neighbor.x, neighbor.y, neighbor.z + 1 );
				}
			} // for neighbors

			prev_wavefront.pop_back();
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
