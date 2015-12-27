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
	h_PathFinder(const h_World& world);
	~h_PathFinder();
	h_PathFinder& operator=(const h_PathFinder&) = delete;

	bool FindPath(
		int src_x, int src_y, int src_z,
		int target_x, int target_y, int target_z );

	const h_PathPoint* GetPathPoints() const;
	unsigned int GetPathLength() const;

private:
	void ClearVisitedCells();
	void SetPointVisited( int x, int y, int z );
	bool IsPointVisited( int x, int y, int z ) const;

private:
	static const constexpr unsigned int max_search_distance_= 13;
	static const constexpr unsigned int cube_size_= 32;

	const h_World& world_;

	h_PathPoint path_[max_search_distance_];
	unsigned int path_length_;

	int cube_position_[3];
	unsigned char visited_cells_bitmap_[ cube_size_ * cube_size_ * cube_size_ / 8 ];
};
