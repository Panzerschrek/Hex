#pragma once
#include "block_collision.hpp"
#include "chunk.hpp"

struct h_ChunkPhysMesh
{
	m_Collection< p_BlockSide > block_sides;
	m_Collection< p_UpperBlockFace > upper_block_faces;

	short z_min, z_max;
};
