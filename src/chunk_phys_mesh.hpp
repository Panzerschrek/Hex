#pragma once
#include "block_collision.hpp"
#include "chunk.hpp"

class h_ChunkPhysMesh
{
public:
	void BuildMesh( h_Chunk* chunk, h_Chunk* chunk_front, h_Chunk *chunk_right,h_Chunk *chunk_back_right,h_Chunk *chunk_back,
					short z_min, short z_max );

	m_Collection< p_BlockSide > block_sides;
	m_Collection< p_UpperBlockFace > upper_block_faces;

	short z_min, z_max;

};
