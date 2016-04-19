#pragma once
#include <vector>

#include "../fwd.hpp"

struct r_FireMeshVertex
{
	float pos[3];
	float tex_coord[2];
	float power;

	r_FireMeshVertex()= default;
	r_FireMeshVertex( float x, float y, float z, float u, float v );
};

void rGenChunkFireMesh( const h_Chunk& chunk, std::vector<r_FireMeshVertex>& out_vertices );
