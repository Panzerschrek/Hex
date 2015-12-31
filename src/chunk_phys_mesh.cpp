#include "chunk_phys_mesh.hpp"


h_ChunkPhysMesh::h_ChunkPhysMesh( h_ChunkPhysMesh&& other )
{
	*this= std::move(other);
}

h_ChunkPhysMesh& h_ChunkPhysMesh::operator=( h_ChunkPhysMesh&& other )
{
	block_sides= std::move(other.block_sides);
	other.block_sides.clear();

	upper_block_faces= std::move(other.upper_block_faces);
	other.upper_block_faces.clear();

	return *this;
}
