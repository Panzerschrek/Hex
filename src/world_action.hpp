#pragma once

struct h_WorldAction
{
	enum class Type
	{
		Build,
		Destroy,
	};

	Type type;
	short coord[3];
	h_BlockType block_type;
};

