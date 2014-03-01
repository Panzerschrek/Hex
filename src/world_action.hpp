#ifndef WORLD_ACTION_HPP
#define WORLD_ACTION_HPP

enum h_ActionType: unsigned char
{
	ACTION_BUILD= 0,
	ACTION_DESTROY
};

struct h_WorldAction
{
	h_ActionType type;
	short coord[3];
	h_BlockType block_type;
};


#endif//WORLD_ACTION_HPP
