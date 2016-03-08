#pragma once
#include <vector>

#include "block_collision.hpp"

// data for player-world physical interaction
struct p_WorldPhysMesh
{
	p_WorldPhysMesh(){}
	p_WorldPhysMesh( const p_WorldPhysMesh& )= delete;
	p_WorldPhysMesh( p_WorldPhysMesh&& other );

	p_WorldPhysMesh& operator=( const p_WorldPhysMesh& )= delete;
	p_WorldPhysMesh& operator=( p_WorldPhysMesh&& other );


	std::vector< p_BlockSide > block_sides;
	std::vector< p_UpperBlockFace > upper_block_faces;
	std::vector< p_WaterBlock > water_blocks;
};
