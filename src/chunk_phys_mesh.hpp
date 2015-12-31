#pragma once
#include <vector>

#include "block_collision.hpp"

struct h_ChunkPhysMesh
{
	h_ChunkPhysMesh(){}
	h_ChunkPhysMesh( const h_ChunkPhysMesh& )= delete;
	h_ChunkPhysMesh( h_ChunkPhysMesh&& other );

	h_ChunkPhysMesh& operator=( const h_ChunkPhysMesh& )= delete;
	h_ChunkPhysMesh& operator=( h_ChunkPhysMesh&& other );


	std::vector< p_BlockSide > block_sides;
	std::vector< p_UpperBlockFace > upper_block_faces;
};
