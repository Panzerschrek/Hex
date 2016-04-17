#include "fire_mesh.hpp"

#include "../chunk.hpp"
#include "../world.hpp"

r_FireMeshVertex::r_FireMeshVertex( float x, float y, float z, float u, float v )
{
	pos[0]= x;
	pos[1]= y;
	pos[2]= z;
	tex_coord[0]= u;
	tex_coord[1]= v;
}

static const float g_x[4]=
{
	0.0f,
	H_HEXAGON_EDGE_SIZE * 0.5f,
	H_HEXAGON_EDGE_SIZE * 1.5f,
	H_HEXAGON_EDGE_SIZE * 2.0f,
};

static const float g_center_x= H_HEXAGON_EDGE_SIZE;

static const float g_up_offset= 0.1f;

static const float g_vecs_to_center[6][2]=
{
	{  1.0f * g_up_offset,  0.0f * g_up_offset },
	{  0.5f * g_up_offset, -H_SPACE_SCALE_VECTOR_X * g_up_offset },
	{ -0.5f * g_up_offset, -H_SPACE_SCALE_VECTOR_X * g_up_offset },
	{ -1.0f * g_up_offset,  0.0f * g_up_offset },
	{ -0.5f * g_up_offset,  H_SPACE_SCALE_VECTOR_X * g_up_offset },
	{  0.5f * g_up_offset,  H_SPACE_SCALE_VECTOR_X * g_up_offset },
};

static const r_FireMeshVertex g_sides_fire_mesh_vertices[6][4]=
{
	{ // Forward
		r_FireMeshVertex( g_x[1], 1.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[2], 1.0f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[2] + g_vecs_to_center[2][0], 1.0f + g_vecs_to_center[2][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] + g_vecs_to_center[1][0], 1.0f + g_vecs_to_center[1][1], 1.0f,   0.0f, 1.0f ),
	},
	{ // Back
		r_FireMeshVertex( g_x[1], 0.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[2], 0.0f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[2] + g_vecs_to_center[4][0], 0.0f + g_vecs_to_center[4][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] + g_vecs_to_center[5][0], 0.0f + g_vecs_to_center[5][1], 1.0f,   0.0f, 1.0f ),
	},
	{ // forward right
		r_FireMeshVertex( g_x[2], 1.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[3], 0.5f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[3] + g_vecs_to_center[3][0], 0.5f + g_vecs_to_center[3][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[2] + g_vecs_to_center[2][0], 1.0f + g_vecs_to_center[2][1], 1.0f,   0.0f, 1.0f ),
	},
	{ // Back left
		r_FireMeshVertex( g_x[1], 0.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[0], 0.5f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[0] + g_vecs_to_center[0][0], 0.5f + g_vecs_to_center[0][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] + g_vecs_to_center[5][0], 0.0f + g_vecs_to_center[5][1], 1.0f,   0.0f, 1.0f ),
	},
	{ // forward left
		r_FireMeshVertex( g_x[1], 1.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[0], 0.5f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[0] + g_vecs_to_center[0][0], 0.5f + g_vecs_to_center[0][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] + g_vecs_to_center[1][0], 1.0f + g_vecs_to_center[1][1], 1.0f,   0.0f, 1.0f ),
	},
	{ // back right
		r_FireMeshVertex( g_x[2], 0.0f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[3], 0.5f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[3] + g_vecs_to_center[3][0], 0.5f + g_vecs_to_center[3][1], 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[2] + g_vecs_to_center[4][0], 0.0f + g_vecs_to_center[4][1], 1.0f,   0.0f, 1.0f ),
	},
};

static const r_FireMeshVertex g_up_side_fire_mesh_vertices[3][4]=
{
	{
		r_FireMeshVertex( g_center_x, 0.5f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[1]    , 1.0f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[1]    , 1.0f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f,   0.0f, 1.0f ),
	},
	{
		r_FireMeshVertex( g_center_x, 0.5f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[3]    , 0.5f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[3]    , 0.5f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f,   0.0f, 1.0f ),
	},
	{
		r_FireMeshVertex( g_center_x, 0.5f, 0.0f,   0.0f, 0.0f ),
		r_FireMeshVertex( g_x[1]    , 0.0f, 0.0f,   1.0f, 0.0f ),
		r_FireMeshVertex( g_x[1]    , 0.0f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f,   0.0f, 1.0f ),
	},
};

static const float g_tc= 1.0f - H_HEXAGON_EDGE_SIZE;

static const r_FireMeshVertex g_down_side_fire_mesh_vertices[3][4]=
{
	{
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f - g_up_offset,   g_tc, g_tc ),
		r_FireMeshVertex( (g_x[1] + g_x[2]) * 0.5f, 1.0f, 1.0f,   g_tc, 1.0f ),
		r_FireMeshVertex( g_x[1], 1.0f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1], 0.5f, 1.0f - g_up_offset,   1.0f, g_tc ),
	},
	{
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f - g_up_offset,   g_tc, g_tc ),
		r_FireMeshVertex( (g_x[2] + g_x[3]) * 0.5f, 0.25f, 1.0f,   g_tc, 1.0f ),
		r_FireMeshVertex( g_x[3], 0.5f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] * 0.25f + g_x[2] * 0.75f, 0.75f, 1.0f - g_up_offset,   1.0f, g_tc ),
	},
	{
		r_FireMeshVertex( g_center_x, 0.5f, 1.0f - g_up_offset,   g_tc, g_tc ),
		r_FireMeshVertex( (g_x[0] + g_x[1]) * 0.5f, 0.25f, 1.0f,   g_tc, 1.0f ),
		r_FireMeshVertex( g_x[1], 0.0f, 1.0f,   1.0f, 1.0f ),
		r_FireMeshVertex( g_x[1] + H_HEXAGON_EDGE_SIZE * 3.0f / 4.0f, 0.25f, 1.0f - g_up_offset,   1.0f, g_tc ),
	},
};

void rGenChunkFireMesh( const h_Chunk& chunk, std::vector<r_FireMeshVertex>& out_vertices )
{
	bool is_edge_chunk= chunk.IsEdgeChunk();

	const h_World& world= *chunk.GetWorld();

	int X= chunk.Longitude() << H_CHUNK_WIDTH_LOG2;
	int Y= chunk.Latitude () << H_CHUNK_WIDTH_LOG2;

	int loaded_zone_X= ( chunk.Longitude() - world.Longitude() ) << H_CHUNK_WIDTH_LOG2;
	int loaded_zone_Y= ( chunk.Latitude () - world.Latitude () ) << H_CHUNK_WIDTH_LOG2;

	const std::vector< h_Fire* >& fire_list= chunk.GetFireList();

	for( const h_Fire* fire : fire_list )
	{
		float x= float( X + fire->x_ ) * H_SPACE_SCALE_VECTOR_X;
		float y= float( Y + fire->y_ ) + 0.5f * float( (fire->x_^1) & 1 );
		float z= float( fire->z_ );
		float power= float(fire->power_) / float(h_Fire::c_max_power_);

		auto fill_quad=
		[ &x, &y, &z, &power]( r_FireMeshVertex* v, const r_FireMeshVertex* v_src )
		{
			for( unsigned int i= 0; i < 4; i++ )
			{
				v[i].pos[0]= x + v_src[i].pos[0];
				v[i].pos[1]= y + v_src[i].pos[1];
				v[i].pos[2]= z + v_src[i].pos[2];

				v[i].tex_coord[0]= v_src[i].tex_coord[0];
				v[i].tex_coord[1]= v_src[i].tex_coord[1];

				v[i].power= power;
			}
		};

		// Up Side. Also, if chuns is edge, draw only up sise.
		if( is_edge_chunk ||
			chunk.GetBlock( fire->x_, fire->y_, fire->z_ - 1 )->
			Flammability() > 0 )
		{
			out_vertices.resize( out_vertices.size() + 3 * 4 );
			r_FireMeshVertex* v= out_vertices.data() + ( out_vertices.size() - 3 * 4 );
			for( unsigned int i= 0; i < 3; i++, v+= 4 )
				fill_quad( v, g_up_side_fire_mesh_vertices[i] );
		}

		if( is_edge_chunk ) continue;

		 // Down side.
		if( chunk.GetBlock( fire->x_, fire->y_, fire->z_ + 1 )->
			Flammability() > 0 )
		{
			out_vertices.resize( out_vertices.size() + 3 * 4 );
			r_FireMeshVertex* v= out_vertices.data() + ( out_vertices.size() - 3 * 4 );
			for( unsigned int i= 0; i < 3; i++, v+= 4 )
				fill_quad( v, g_down_side_fire_mesh_vertices[i] );
		}

		int loaded_zone_x= loaded_zone_X + fire->x_;
		int loaded_zone_y= loaded_zone_Y + fire->y_;

		int forward_side_y= loaded_zone_y + ( (loaded_zone_x^1) & 1 );
		int back_side_y= loaded_zone_y - (loaded_zone_x & 1);

		int neighbors[][2]=
		{
			{ loaded_zone_x, loaded_zone_y + 1 },
			{ loaded_zone_x, loaded_zone_y - 1 },
			{ loaded_zone_x + 1, forward_side_y },
			{ loaded_zone_x - 1, back_side_y },
			{ loaded_zone_x - 1, forward_side_y },
			{ loaded_zone_x + 1, back_side_y },
		};

		for( unsigned int side= 0; side < 6; side++ )
		{
			if( world.GetChunk(
					neighbors[side][0] >> H_CHUNK_WIDTH_LOG2,
					neighbors[side][1] >> H_CHUNK_WIDTH_LOG2 )->
				GetBlock(
					neighbors[side][0] & (H_CHUNK_WIDTH - 1),
					neighbors[side][1] & (H_CHUNK_WIDTH - 1),
					fire->z_ )->
				Flammability() > 0 )
			{
				out_vertices.resize( out_vertices.size() + 4 );
				fill_quad(
					out_vertices.data() + ( out_vertices.size() - 4 ),
					g_sides_fire_mesh_vertices[side] );
			}
		} // for sides
	} // for fire blocks
}
