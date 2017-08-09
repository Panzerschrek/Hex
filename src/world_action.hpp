#pragma once
#include "hex.hpp"

struct h_WorldAction
{
	enum class Type
	{
		Build,
		Destroy,
	};

	Type type;

	h_BlockType block_type;

	h_Direction horizontal_direction;
	h_Direction vertical_direction;

	int coord[3];
};

