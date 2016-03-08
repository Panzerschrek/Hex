#include "world_phys_mesh.hpp"


p_WorldPhysMesh::p_WorldPhysMesh( p_WorldPhysMesh&& other )
{
	*this= std::move(other);
}

p_WorldPhysMesh& p_WorldPhysMesh::operator=( p_WorldPhysMesh&& other )
{
	block_sides= std::move(other.block_sides);
	other.block_sides.clear();

	upper_block_faces= std::move(other.upper_block_faces);
	other.upper_block_faces.clear();

	water_blocks= std::move(other.water_blocks);
	other.water_blocks.clear();

	return *this;
}
