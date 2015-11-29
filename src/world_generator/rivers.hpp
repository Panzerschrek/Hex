#pragma once
#include <vector>

#include "../math_lib/fixed.hpp"

struct g_RiverPoint
{
	fixed8_t x, y;
	unsigned short river_id;
	unsigned short index_in_river_segment;
};

struct g_RiverSegment
{
	std::vector< unsigned short > points_indeces;

	// Nonzero id`s of related segments
	unsigned short parents_id[2];
	unsigned short child_id;
	bool too_short;
};


struct g_RiverSystem
{
	std::vector< g_RiverPoint > rivers_points;
	std::vector< g_RiverSegment > rivers_segments;
};


