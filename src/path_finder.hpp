#pragma once

#include "hex.hpp"
#include "fwd.hpp"

struct h_PathPoint
{
	int x, y, z;
};

class h_PathFinder
{
public:
	h_PathFinder( const h_World& world );
	~h_PathFinder();
	h_PathFinder& operator=( const h_PathFinder& )= delete;

	bool FindPath(
		int src_x, int src_y, int src_z,
		int target_x, int target_y, int target_z,
		unsigned int max_radius= GetMaxSearchRadius() );

	// Get Result Path.
	// Path contains cells from source to target, include target, but exclude source.
	const h_PathPoint* GetPathPoints() const;
	unsigned int GetPathLength() const;

	static unsigned int GetMaxSearchRadius();

private:
	void ClearVisitedCells();
	void SetPointVisited( int x, int y, int z );
	bool IsPointVisited( int x, int y, int z ) const;
	int GetBitmapAddress( int x, int y, int z ) const;

private:
	static const constexpr unsigned int c_cube_size_log2_= 6;
	static const constexpr unsigned int c_cube_size_= 1 << c_cube_size_log2_;
	static const constexpr unsigned int c_max_search_distance_= (c_cube_size_ >> 1) - 2;

	const h_World& world_;

	h_PathPoint path_[ c_max_search_distance_ ];
	unsigned int path_length_;

	int cube_position_[3];
	unsigned char visited_cells_bitmap_[ ( c_cube_size_ * c_cube_size_ * c_cube_size_ ) >> 3 ];
};
