#pragma once

#pragma pack( push, 1 )

struct r_WorldVertex
{
	short coord[3];
	short tex_coord[3];
	unsigned char light[2];
	unsigned char normal_id;
	char reserved[1];
};//16b struct

struct r_WaterVertex
{
	short coord[3];
	unsigned char light[2];
};//8b struct

#pragma pack( pop )


class r_ChunkInfo
{
public:
	r_ChunkInfo();

	void GetWaterHexCount();
	void BuildWaterSurfaceMesh();

	void GetQuadCount();
	void BuildChunkMesh();

	// Pointer to external storage for vertices.
	r_WorldVertex* vertex_data_= nullptr;
	unsigned int vertex_count_= 0;
	// ChunkInfo is always updated after creation.
	bool updated_= true;

	r_WaterVertex* water_vertex_data_= nullptr;
	unsigned int water_vertex_count_= 0;
	bool water_updated_= true;

	// Flags, setted by world.
	// Chunk can really updates later, after this flags setted.
	bool update_requested_= false;
	bool water_update_requested_= false;

	//geomentry up and down range borders. Used only for generation of center chunk blocks( not for border blocks )
	int max_geometry_height_, min_geometry_height_;

	const h_Chunk* chunk_;
	const h_Chunk* chunk_front_, *chunk_right_, *chunk_back_right_, *chunk_back_;
};