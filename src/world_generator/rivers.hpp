#pragma once
#include <vector>

#include "../math_lib/fixed.hpp"

struct g_RiverPoint
{
	fixed8_t x, y;
	unsigned short river_id;
};

struct g_RiverSegment
{
	std::vector< unsigned short > points_indeces;

	// Nonzero id`s of related segments
	unsigned short parents_id[2];
	unsigned short child_id;
};


struct g_RiverSystem
{
	std::vector< g_RiverPoint > rivers_points;
	std::vector< g_RiverSegment > rivers_segments;
};


