#include "../world.hpp"

#include "chunk_info.hpp"
#include "texture_manager.hpp"
#include "rendering_constants.hpp"

static bool IsWaterBlockVisible(
	const h_LiquidBlock* water_block,
	const h_Block* upper_block )
{
	return
		upper_block->Type() == h_BlockType::Air ||
		( upper_block->Type() != h_BlockType::Water && water_block->LiquidLevel() < H_MAX_WATER_LEVEL );
}

r_ChunkInfo::r_ChunkInfo()
	: chunk_front_(nullptr), chunk_right_(nullptr)
	, chunk_back_right_(nullptr), chunk_back_(nullptr)
{
}

void r_ChunkInfo::GetWaterHexCount()
{
	const std::vector< h_LiquidBlock* >& water_block_list= chunk_->GetWaterList();

	unsigned int hex_count= 0;

	for( const h_LiquidBlock* b : water_block_list )
	{
		if( IsWaterBlockVisible( b, chunk_->GetBlock( b->x_, b->y_, b->z_ + 1 ) ) )
			hex_count++;
	}

	water_vertex_count_= hex_count * 6;
}

void r_ChunkInfo::BuildWaterSurfaceMesh()
{
	const std::vector< h_LiquidBlock* >& water_block_list= chunk_->GetWaterList();

	const h_World* world= chunk_->GetWorld();

	short X= chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	short Y= chunk_->Latitude()  << H_CHUNK_WIDTH_LOG2;

	short chunk_loaded_zone_X= ( chunk_->Longitude() - world->Longitude() ) << H_CHUNK_WIDTH_LOG2;
	short chunk_loaded_zone_Y= ( chunk_->Latitude () - world->Latitude () ) << H_CHUNK_WIDTH_LOG2;

	r_WaterVertex* v= water_vertex_data_;

	if( !chunk_->IsEdgeChunk() )
	{
		for( const h_LiquidBlock* b : water_block_list )
		{
			if( IsWaterBlockVisible( b, chunk_->GetBlock( b->x_, b->y_, b->z_ + 1 ) ) )
			{
				v[0].coord[0]= 3 * ( b->x_ + X );
				v[1].coord[0]= v[5].coord[0]= v[0].coord[0] + 1;
				v[2].coord[0]= v[4].coord[0]= v[0].coord[0] + 3;
				v[3].coord[0]= v[0].coord[0] + 4;

				v[0].coord[1]= v[3].coord[1]= 2 * ( b->y_ + Y ) - ((b->x_)&1) + 2;
				v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
				v[4].coord[1]= v[5].coord[1]= v[0].coord[1] - 1;

				// Calculate height of vertices.
				bool upper_block_is_water[6]= { false, false, false, false, false, false };
				bool nearby_block_is_air [6]= { false, false, false, false, false, false };
				unsigned int vertex_water_level[6];
				unsigned short vertex_water_block_count[6];
				vertex_water_level[0]= vertex_water_level[1]= vertex_water_level[2]=
				vertex_water_level[3]= vertex_water_level[4]= vertex_water_level[5]= b->LiquidLevel();
				vertex_water_block_count[0]= vertex_water_block_count[1]= vertex_water_block_count[2]=
				vertex_water_block_count[3]= vertex_water_block_count[4]= vertex_water_block_count[5]= 1;

				int global_x= chunk_loaded_zone_X + b->x_;
				int global_y= chunk_loaded_zone_Y + b->y_;
				int forward_side_y= global_y + ( (global_x^1) & 1 );
				int back_side_y= global_y - (global_x & 1);

				int neighbors[6][2]=
				{
					{ global_x - 1, forward_side_y },
					{ global_x, global_y + 1 },
					{ global_x + 1, forward_side_y },
					{ global_x + 1, back_side_y },
					{ global_x, global_y - 1 },
					{ global_x - 1, back_side_y },
				};

				for( unsigned int d= 0; d < 6; d++ )
				{
					int local_x= neighbors[d][0] & (H_CHUNK_WIDTH - 1);
					int local_y= neighbors[d][1] & (H_CHUNK_WIDTH - 1);

					const h_Chunk* ch2=
						world->GetChunk(
							neighbors[d][0] >> H_CHUNK_WIDTH_LOG2,
							neighbors[d][1] >> H_CHUNK_WIDTH_LOG2 );

					unsigned int addr= BlockAddr( local_x, local_y, b->z_ );
					const h_Block* b2= ch2->GetBlock( addr );
					const h_Block* b3= ch2->GetBlock( addr+ 1 );

					static const unsigned int c_next_vi[6]= { 1, 2, 3, 4, 5, 0 };
					unsigned int vi0= d;
					unsigned int vi1= c_next_vi[d];

					if( b3->Type() == h_BlockType::Water )
						upper_block_is_water[vi0]= upper_block_is_water[vi1]= true;
					else if( b2->Type() == h_BlockType::Air )
						nearby_block_is_air[vi0]= nearby_block_is_air[vi1]= true;
					else if( b2->Type() == h_BlockType::Water )
					{
						auto water_block= static_cast<const h_LiquidBlock*>(b2);

						vertex_water_level[vi0]+= water_block->LiquidLevel();
						vertex_water_level[vi1]+= water_block->LiquidLevel();
						vertex_water_block_count[ vi0 ]++;
						vertex_water_block_count[ vi1 ]++;
					}
				} // for neighbors

				for( unsigned int k= 0; k< 6; k++ )
				{
					// For fast division ( but not precise ).
					static const constexpr unsigned int c_div_table_shift= 14;
					static const constexpr unsigned int c_div_table_scaler= 1 << c_div_table_shift;
					static const unsigned int div_table[7]=
					{
						0,
						c_div_table_scaler/1, c_div_table_scaler/2, c_div_table_scaler/3,
						c_div_table_scaler/4, c_div_table_scaler/5, c_div_table_scaler/6,
					};

					if( upper_block_is_water[k] )
						v[k].coord[2]= (b->z_+1) << R_WATER_VERTICES_Z_SCALER_LOG2;
					else if( nearby_block_is_air[k] )
						v[k].coord[2]= b->z_ << R_WATER_VERTICES_Z_SCALER_LOG2;
					else
					{
						unsigned int avg_water_level=
							( ( vertex_water_level[k] * div_table[ vertex_water_block_count[k] ] ) >> c_div_table_shift );

						v[k].coord[2]=
							( b->z_ << R_WATER_VERTICES_Z_SCALER_LOG2 )+
							( avg_water_level >> ( H_MAX_WATER_LEVEL_LOG2 - R_WATER_VERTICES_Z_SCALER_LOG2 ) );
					}
				} // for vertices

				world->GetForwardVertexLight( b->x_ + chunk_loaded_zone_X - 1, b->y_ + chunk_loaded_zone_Y - (b->x_&1), b->z_, v[0].light );
				world->GetBackVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y + 1, b->z_, v[1].light );
				world->GetForwardVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y, b->z_, v[2].light );
				world->GetBackVertexLight( b->x_ + chunk_loaded_zone_X + 1, b->y_ + chunk_loaded_zone_Y + ((1+b->x_)&1), b->z_, v[3].light );
				world->GetForwardVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y - 1, b->z_, v[4].light );
				world->GetBackVertexLight(  b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y, b->z_, v[5].light );

				v+= 6;
			}
		}

	}//smooth water surface
	else
	{
		for( const h_LiquidBlock* b : water_block_list )
		{
			if( IsWaterBlockVisible( b, chunk_->GetBlock( b->x_, b->y_, b->z_ + 1 ) ) )
			{
				v[0].coord[0]= 3 * ( b->x_ + X );
				v[1].coord[0]= v[5].coord[0]= v[0].coord[0] + 1;
				v[2].coord[0]= v[4].coord[0]= v[0].coord[0] + 3;
				v[3].coord[0]= v[0].coord[0] + 4;

				v[0].coord[1]= v[3].coord[1]= 2 * ( b->y_ + Y ) - ((b->x_)&1) + 2;
				v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
				v[4].coord[1]= v[5].coord[1]= v[0].coord[1] - 1;

				short h=
					( b->z_ << R_WATER_VERTICES_Z_SCALER_LOG2 ) +
					( b->LiquidLevel() >> ( H_MAX_WATER_LEVEL_LOG2 - R_WATER_VERTICES_Z_SCALER_LOG2 ) );

				v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[4].coord[2]= v[5].coord[2]= h;

				unsigned char light[2][2];
				chunk_->GetLightsLevel( b->x_, b->y_, b->z_ + 0, light[0] );
				chunk_->GetLightsLevel( b->x_, b->y_, b->z_ + 1, light[1] );
				v[0].light[0]= v[1].light[0]= v[2].light[0]=
				v[3].light[0]= v[4].light[0]= v[5].light[0]=
					std::max( light[0][0], light[1][0] ) << 4;

				v[0].light[1]= v[1].light[1]= v[2].light[1]=
				v[3].light[1]= v[4].light[1]= v[5].light[1]=
					std::max( light[0][1], light[1][1] ) << 4;
				v+= 6;
			}// if water surface
		}//for
	}

	H_ASSERT( v == water_vertex_data_ + water_vertex_count_ );
}

/*
 __    __
/ @\__/ @\__
\__/ @\__/ &\
/ d\__/ d\__/
\__/ d\__/ =\
/ d\__/ d\__/
\__/ d\__/ =\
/ *\__/ *\__/
\__/ *\__/ +\
   \__/  \__/

 f
_____
     \ fr
  up  \
      /
     / br

d - default stage
@ - front edge
* - back edge
= - right edge
&,+ - corners
*/

void r_ChunkInfo::GetQuadCount()
{
	unsigned int quad_count= 0;

	min_geometry_height_= H_CHUNK_HEIGHT;
	max_geometry_height_= 0;

	for( int x= 0; x< H_CHUNK_WIDTH; x++ )
	for( int y= 0; y< H_CHUNK_WIDTH; y++ )
	{
		const unsigned char* t_p= chunk_->GetTransparencyData() + BlockAddr(x,y,0);
		const unsigned char* t_fr_p= chunk_->GetTransparencyData() + BlockAddr(x + 1, y + (1&(x+1)),0);
		const unsigned char* t_br_p= chunk_->GetTransparencyData() + BlockAddr(x + 1, y - ( 1&x )  ,0);
		const unsigned char* t_f_p= chunk_->GetTransparencyData() + BlockAddr(x,y+1,0);

		//front chunk border
		if( y == H_CHUNK_WIDTH - 1 && x < H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
				t_f_p= chunk_front_->GetTransparencyData() + BlockAddr( x, 0, 0 );
			else
				t_f_p= t_p;//this block transparency
			if(x&1)
				t_fr_p= chunk_->GetTransparencyData() + BlockAddr( x + 1, H_CHUNK_WIDTH - 1, 0 );
			else if( chunk_front_ != nullptr )
				t_fr_p= chunk_front_->GetTransparencyData() + BlockAddr( x + 1, 0, 0 );
			else
				t_fr_p= t_p;//this block transparency
		}
		//back chunk border
		else if( y == 0 && x < H_CHUNK_WIDTH - 1 )
		{
			if(!(x&1))
				t_br_p= chunk_->GetTransparencyData() + BlockAddr( x + 1, 0, 0 );
			else if( chunk_back_ != nullptr )
				t_br_p= chunk_back_->GetTransparencyData() + BlockAddr( x+ 1, H_CHUNK_WIDTH - 1, 0 );
			else
				t_br_p= t_p;//this block transparency
		}
		//right chunk border
		else if( x == H_CHUNK_WIDTH - 1 && y> 0 && y< H_CHUNK_WIDTH-1 )
		{
			if( chunk_right_ != nullptr )
			{
				t_fr_p= chunk_right_->GetTransparencyData() + BlockAddr( 0, y, 0 );
				t_br_p= chunk_right_->GetTransparencyData() + BlockAddr( 0, y - 1, 0 );
			}
			else
				t_fr_p= t_br_p= t_p;//this block transparency
		}
		else if( x == H_CHUNK_WIDTH - 1 && y == H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
				t_f_p= chunk_front_->GetTransparencyData() + BlockAddr( H_CHUNK_WIDTH - 1, 0, 0 );
			else
				t_f_p= t_p;//this block transparency;
			if( chunk_right_ != nullptr )
			{
				t_fr_p= chunk_right_->GetTransparencyData() + BlockAddr( 0, H_CHUNK_WIDTH  - 1, 0 );
				t_br_p= chunk_right_->GetTransparencyData() + BlockAddr( 0, H_CHUNK_WIDTH  - 2, 0 );
			}
			else
				t_fr_p= t_br_p= t_p;//this block transparency;
		}
		else if( x == H_CHUNK_WIDTH - 1 && y == 0 )
		{
			if( chunk_right_ != nullptr )
				t_fr_p= chunk_right_->GetTransparencyData() + BlockAddr( 0, 0, 0 );
			else
				t_fr_p= t_p;//this block transparency;
			if( chunk_back_right_ != nullptr )
				t_br_p= chunk_back_right_->GetTransparencyData() + BlockAddr( 0, H_CHUNK_WIDTH - 1, 0 );
			else
				t_br_p= t_p;//this block transparency;
		}
		int column_max_geometry_height= 0;
		for( int z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
		{
			unsigned char t= t_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_fr= t_fr_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_br= t_br_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_up= t_p[z+1] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_f= t_f_p[z] & H_VISIBLY_TRANSPARENCY_BITS;

			if( t != t_up )
			{
				quad_count+=2;
				column_max_geometry_height= z;
				if( z < min_geometry_height_ ) min_geometry_height_= z;
			}
			if( t != t_fr )
			{
				quad_count++;
				column_max_geometry_height= z;
				if( z < min_geometry_height_ ) min_geometry_height_= z;
			}
			if( t != t_br )
			{
				quad_count++;
				column_max_geometry_height= z;
				if( z < min_geometry_height_ ) min_geometry_height_= z;
			}
			if( t!= t_f )
			{
				quad_count++;
				column_max_geometry_height= z;
				if( z < min_geometry_height_ ) min_geometry_height_= z;
			}
		}//for z

		if( column_max_geometry_height > max_geometry_height_ )
			max_geometry_height_= column_max_geometry_height;
	}//for xy

	quad_count+= GetNonstandardFormBlocksQuadCount();

	vertex_count_= quad_count * 4;
}

/*
                       1____2
                       /    \
                    05/______\36
                      \      /
                      4\____/7
up/down side */

/*
                           2
                          /\
                     ____/  \3
                    /   1\  /
                   /______\/0
                   \      /
                    \____/
forward right side*/

/*
                     ____
                    /    \
                   /______\0
                   \      /\
                    \___3/  \1
                         \  /
                          \/
                           2
back right*/

/*
                       1+--+2 - z
                        |  |
                        |  |
                       0+__+3 - z + 1
                       /    \
                      /______\
                      \      /
                       \____/
forward side*/

void r_ChunkInfo::BuildChunkMesh()
{
	r_WorldVertex* v= vertex_data_;

	const h_World* w= chunk_->GetWorld();
	int X= chunk_->Longitude() * H_CHUNK_WIDTH;
	int Y= chunk_->Latitude () * H_CHUNK_WIDTH;
	int relative_X= ( chunk_->Longitude() - w->Longitude() ) * H_CHUNK_WIDTH;
	int relative_Y= ( chunk_->Latitude () - w->Latitude () ) * H_CHUNK_WIDTH;

	bool flat_lighting= chunk_->IsEdgeChunk();

	for( int x= 0; x< H_CHUNK_WIDTH; x++ )
	for( int y= 0; y< H_CHUNK_WIDTH; y++ )
	{
		int offset;

		offset= BlockAddr(x,y,0);
		const unsigned char* t_p= chunk_->GetTransparencyData() + offset;
		const h_Block* const* b_p= chunk_->GetBlocksData() + offset;
		const unsigned char* ls_p= chunk_->GetSunLightData() + offset;
		const unsigned char* lf_p= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x + 1, y + (1&(x+1)),0);
		const unsigned char* t_fr_p= chunk_->GetTransparencyData() + offset;
		const h_Block* const* b_fr_p= chunk_->GetBlocksData() + offset;
		const unsigned char* ls_fr_p= chunk_->GetSunLightData() + offset;
		const unsigned char* lf_fr_p= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x + 1, y - ( 1&x )  ,0);
		const unsigned char* t_br_p= chunk_->GetTransparencyData() + offset;
		const h_Block* const* b_br_p= chunk_->GetBlocksData() + offset;
		const unsigned char* ls_br_p= chunk_->GetSunLightData() + offset;
		const unsigned char* lf_br_p= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x,y+1,0);
		const unsigned char* t_f_p= chunk_->GetTransparencyData() + offset;
		const h_Block* const* b_f_p= chunk_->GetBlocksData() + offset;
		const unsigned char* ls_f_p= chunk_->GetSunLightData() + offset;
		const unsigned char* lf_f_p= chunk_->GetFireLightData() + offset;

		//front chunk border
		if( y == H_CHUNK_WIDTH - 1 && x < H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
			{
				offset= BlockAddr( x, 0, 0 );
				t_f_p= chunk_front_->GetTransparencyData() + offset;
				b_f_p= chunk_front_->GetBlocksData() + offset;
				ls_f_p= chunk_front_->GetSunLightData() + offset;
				lf_f_p= chunk_front_->GetFireLightData() + offset;
			}
			else
				t_f_p= t_p;//this block transparency

			if(x&1)
			{
				offset= BlockAddr( x + 1, H_CHUNK_WIDTH - 1, 0 );
				t_fr_p= chunk_->GetTransparencyData() + offset;
				b_fr_p= chunk_->GetBlocksData() + offset;
				ls_fr_p= chunk_->GetSunLightData() + offset;
				lf_fr_p= chunk_->GetFireLightData() + offset;
			}
			else if( chunk_front_ != nullptr )
			{
				offset= BlockAddr( x + 1, 0, 0 );
				t_fr_p= chunk_front_->GetTransparencyData() + offset;
				b_fr_p= chunk_front_->GetBlocksData() + offset;
				ls_fr_p= chunk_front_->GetSunLightData() + offset;
				lf_fr_p= chunk_front_->GetFireLightData() + offset;
			}
			else
				t_fr_p= t_p;//this block transparency
		}
		//back chunk border
		else if( y == 0 && x < H_CHUNK_WIDTH - 1 )
		{
			if(!(x&1))
			{
				offset= BlockAddr( x + 1, 0, 0 );
				t_br_p= chunk_->GetTransparencyData() + offset;
				b_br_p= chunk_->GetBlocksData() + offset;
				ls_br_p= chunk_->GetSunLightData() + offset;
				lf_br_p= chunk_->GetFireLightData() + offset;
			}
			else if( chunk_back_ != nullptr )
			{
				offset= BlockAddr( x+ 1, H_CHUNK_WIDTH - 1, 0 );
				t_br_p= chunk_back_->GetTransparencyData() + offset;
				b_br_p= chunk_back_->GetBlocksData() + offset;
				ls_br_p= chunk_back_->GetSunLightData() + offset;
				lf_br_p= chunk_back_->GetFireLightData() + offset;
			}
			else
				t_br_p= t_p;//this block transparency
		}
		//right chunk border
		else if( x == H_CHUNK_WIDTH - 1 && y> 0 && y< H_CHUNK_WIDTH-1 )
		{
			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, y, 0 );
				t_fr_p= chunk_right_->GetTransparencyData() + offset;
				b_fr_p= chunk_right_->GetBlocksData() + offset;
				ls_fr_p= chunk_right_->GetSunLightData() + offset;
				lf_fr_p= chunk_right_->GetFireLightData() + offset;

				offset= BlockAddr( 0, y - 1, 0 );
				t_br_p= chunk_right_->GetTransparencyData() + offset;
				b_br_p= chunk_right_->GetBlocksData() + offset;
				ls_br_p= chunk_right_->GetSunLightData() + offset;
				lf_br_p= chunk_right_->GetFireLightData() + offset;

			}
			else
				t_fr_p= t_br_p= t_p;//this block transparency
		}
		else if( x == H_CHUNK_WIDTH - 1 && y == H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, 0, 0 );
				t_f_p= chunk_front_->GetTransparencyData() + offset;
				b_f_p= chunk_front_->GetBlocksData() + offset;
				ls_f_p= chunk_front_->GetSunLightData() + offset;
				lf_f_p= chunk_front_->GetFireLightData() + offset;
			}
			else
				t_f_p= t_p;//this block transparency;
			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, H_CHUNK_WIDTH  - 1, 0 );
				t_fr_p= chunk_right_->GetTransparencyData() + offset;
				b_fr_p= chunk_right_->GetBlocksData() + offset;
				ls_fr_p= chunk_right_->GetSunLightData() + offset;
				lf_fr_p= chunk_right_->GetFireLightData() + offset;

				offset= BlockAddr( 0, H_CHUNK_WIDTH  - 2, 0 );
				t_br_p= chunk_right_->GetTransparencyData() + offset;
				b_br_p= chunk_right_->GetBlocksData() + offset;
				ls_br_p= chunk_right_->GetSunLightData() + offset;
				lf_br_p= chunk_right_->GetFireLightData() + offset;
			}
			else
				t_fr_p= t_br_p= t_p;//this block transparency;
		}
		else if( x == H_CHUNK_WIDTH - 1 && y == 0 )
		{
			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, 0, 0 );
				t_fr_p= chunk_right_->GetTransparencyData() + offset;
				b_fr_p= chunk_right_->GetBlocksData() + offset;
				ls_fr_p= chunk_right_->GetSunLightData() + offset;
				lf_fr_p= chunk_right_->GetFireLightData() + offset;
			}
			else
				t_fr_p= t_p;//this block transparency;
			if( chunk_back_right_ != nullptr )
			{
				offset= BlockAddr( 0, H_CHUNK_WIDTH - 1, 0 );
				t_br_p= chunk_back_right_->GetTransparencyData() + offset;
				b_br_p= chunk_back_right_->GetBlocksData() + offset;
				ls_br_p= chunk_back_right_->GetSunLightData() + offset;
				lf_br_p= chunk_back_right_->GetFireLightData() + offset;
			}
			else
				t_br_p= t_p;//this block transparency;
		}

		for( int z= min_geometry_height_; z<= max_geometry_height_; z++ )
		{
			unsigned char normal_id;
			unsigned char tex_id, tex_scale, light[2];
			const h_Block* b;

			unsigned char t= t_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_fr= t_fr_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_br= t_br_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_up= t_p[z+1] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_f= t_f_p[z] & H_VISIBLY_TRANSPARENCY_BITS;

			if( t != t_up )//up
			{
				if( t > t_up )
				{
					normal_id= static_cast<unsigned char>(h_Direction::Down);
					b= b_p[z+1];
					light[0]= ls_p[z];
					light[1]= lf_p[z];
				}
				else
				{
					normal_id= static_cast<unsigned char>(h_Direction::Up);
					b= b_p[z];
					light[0]= ls_p[z+1];
					light[1]= lf_p[z+1];
				}

				tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
				tex_scale= r_TextureManager::GetTextureScale( tex_id );

				v[0].coord[0]= 3 * ( x + X );
				v[1].coord[0]= v[4].coord[0]= v[0].coord[0] + 1;
				v[2].coord[0]= v[7].coord[0]= v[0].coord[0] + 3;
				v[3].coord[0]= v[0].coord[0] + 4;

				v[0].coord[1]= v[3].coord[1]= 2 * ( y + Y ) - (x&1) + 2;
				v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
				v[7].coord[1]= v[4].coord[1]= v[0].coord[1] - 1;

				v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[7].coord[2]= v[4].coord[2]= (z+1) << 1;

				if( r_TextureManager::TexturePerBlock( tex_id ) )
				{
					v[0].tex_coord[0]= 0;
					v[1].tex_coord[0]= v[4].tex_coord[0]= 1*H_TEXTURE_SCALE_MULTIPLIER;
					v[2].tex_coord[0]= v[7].tex_coord[0]= 3*H_TEXTURE_SCALE_MULTIPLIER;
					v[3].tex_coord[0]= 4*H_TEXTURE_SCALE_MULTIPLIER;

					v[0].tex_coord[1]= v[3].tex_coord[1]= 1*H_TEXTURE_SCALE_MULTIPLIER;
					v[1].tex_coord[1]= v[2].tex_coord[1]= 2*H_TEXTURE_SCALE_MULTIPLIER;
					v[7].tex_coord[1]= v[4].tex_coord[1]= 0;
				}
				else
				{
					v[0].tex_coord[0]= tex_scale * v[0].coord[0];
					v[1].tex_coord[0]= v[4].tex_coord[0]= v[0].tex_coord[0] + 1*tex_scale;
					v[2].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 3*tex_scale;
					v[3].tex_coord[0]= v[0].tex_coord[0] + 4*tex_scale;

					v[0].tex_coord[1]= v[3].tex_coord[1]= tex_scale * v[0].coord[1];
					v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + 1*tex_scale;
					v[7].tex_coord[1]= v[4].tex_coord[1]= v[0].tex_coord[1] - 1*tex_scale;
				}

				//v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= v[7].normal_id= v[4].normal_id= normal_id;
				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= v[7].tex_coord[2]= v[4].tex_coord[2]=
				tex_id;
				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= v[7].light[0]= v[4].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= v[7].light[1]= v[4].light[1]= light[1] << 4;
				}
				else
				{
					w->GetForwardVertexLight( x + relative_X - 1, y + relative_Y - (x&1), z, v[0].light );
					w->GetBackVertexLight( x + relative_X, y + relative_Y + 1, z, v[1].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[2].light );
					w->GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((1+x)&1), z, v[3].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z, v[7].light );
					w->GetBackVertexLight(  x + relative_X, y + relative_Y, z, v[4].light );
				}
				v[5]= v[0];
				v[6]= v[3];

				if( normal_id == static_cast<unsigned char>(h_Direction::Down) )
				{
					std::swap( v[1], v[3] );
					std::swap( v[5], v[7] );
				}
				v+=8;
			}

			if( t != t_fr )//forward right
			{
				if( t > t_fr )
				{
					normal_id= static_cast<unsigned char>(h_Direction::BackLeft);
					b= b_fr_p[z];
					light[0]= ls_p[z];
					light[1]= lf_p[z];
				}
				else
				{
					normal_id= static_cast<unsigned char>(h_Direction::ForwardRight);
					b= b_p[z];
					light[0]= ls_fr_p[z];
					light[1]= lf_fr_p[z];
				}

				tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
				tex_scale= r_TextureManager::GetTextureScale( tex_id );

				v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( x + X ) + 3;
				v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

				v[0].coord[1]= v[ 3 ].coord[1]= 2 * ( y + Y ) - (x&1) + 2;
				v[ 1 ].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;

				v[0].coord[2]= v[ 1 ].coord[2]= (z+1) << 1;
				v[2].coord[2]= v[ 3 ].coord[2]= z << 1;


				v[ 1 ].tex_coord[0]= v[2].tex_coord[0]= tex_scale * ( v[ 1 ].coord[1] - v[1].coord[0] );
				v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[ 1 ].tex_coord[0] - 2 * tex_scale;

				v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * (2 * tex_scale);
				v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= v[0].tex_coord[1] - (2 * tex_scale);

				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;
				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					w->GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z, v[0].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[1].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y, z-1, v[2].light  );
					w->GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z-1, v[3].light  );
				}
				//v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;
				if( normal_id == static_cast<unsigned char>(h_Direction::BackLeft) )
					std::swap( v[1], v[3] );

				v+=4;
			}

			if( t != t_br )//back right
			{
				if( t > t_br )
				{
					normal_id= static_cast<unsigned char>(h_Direction::ForwardLeft);
					b= b_br_p[z];
					light[0]= ls_p[z];
					light[1]= lf_p[z];
				}
				else
				{
					normal_id= static_cast<unsigned char>(h_Direction::BackRight);
					b= b_p[z];
					light[0]= ls_br_p[z];
					light[1]= lf_br_p[z];
				}

				tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
				tex_scale= r_TextureManager::GetTextureScale( tex_id );

				v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( x + X ) + 3;
				v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

				v[ 1 ].coord[1]= v[2].coord[1]= 2 * ( y + Y ) - (x&1) + 2 - 1;
				v[0].coord[1]= v[ 3 ].coord[1]= v[ 1 ].coord[1] + 1;

				v[ 1 ].coord[2]= v[0].coord[2]= (z+1) << 1;
				v[2].coord[2]= v[ 3 ].coord[2]= z << 1;


				v[2].tex_coord[0]= v[ 1 ].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;
				v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[2].tex_coord[0] + 2 * tex_scale;

				v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * (2 * tex_scale);
				v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - (2 * tex_scale);

				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;
				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					w->GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z, v[0].light );
					w->GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z - 1, v[3].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z - 1, v[2].light );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z, v[1].light );
				}
				//v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;
				if( normal_id == static_cast<unsigned char>(h_Direction::BackRight) )
					std::swap( v[1], v[3] );

				v+=4;
			}

			if( t != t_f )//forward
			{
				if( t > t_f )
				{
					normal_id= static_cast<unsigned char>(h_Direction::Back);
					b= b_f_p[z];
					light[0]= ls_p[z];
					light[1]= lf_p[z];
				}
				else
				{
					normal_id= static_cast<unsigned char>(h_Direction::Forward);
					b= b_p[z];
					light[0]= ls_f_p[z];
					light[1]= lf_f_p[z];
				}

				tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
				tex_scale= r_TextureManager::GetTextureScale( tex_id );

				v[0].coord[0]= v[ 1 ].coord[0]= 3 * ( x + X ) + 1;
				v[0].coord[1]= v[ 1 ].coord[1]= v[2].coord[1]= v[ 3 ].coord[1]= 2 * ( y + Y ) - (x&1) + 2 + 1;

				v[0].coord[2]= v[ 3 ].coord[2]= (z+1) << 1;
				v[ 1 ].coord[2]= v[2].coord[2]= z << 1;

				v[ 3 ].coord[0]= v[2].coord[0]= v[ 1 ].coord[0] + 2;


				v[0].tex_coord[0]= v[ 1 ].tex_coord[0]= v[0].coord[0] * tex_scale;
				v[2].tex_coord[0]= v[ 3 ].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;

				v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= z * (2 * tex_scale);
				v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - (2 * tex_scale);
				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;

				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					w->GetBackVertexLight( x + relative_X, y + relative_Y + 1, z, v[0].light );
					w->GetBackVertexLight( x + relative_X, y + relative_Y + 1, z - 1, v[1].light  );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y, z - 1, v[2].light  );
					w->GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[3].light  );
				}
				//v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;
				if( normal_id == static_cast<unsigned char>(h_Direction::Back) )
					std::swap( v[1], v[3] );

				v+= 4;
			}//forward quad
		}//for z
	}//for xy

	v= BuildNonstandardFormBlocks( v );

	H_ASSERT( v - vertex_data_ == (int)vertex_count_ );
}

unsigned int r_ChunkInfo::GetNonstandardFormBlocksQuadCount()
{
	unsigned int quad_count= 0;

	const std::vector< h_NonstandardFormBlock* >& blocks= chunk_->GetNonstandartFormBlocksList();

	for( const h_NonstandardFormBlock* block : blocks )
	{
		switch( h_Block::Form(block->Type()) )
		{
		case h_BlockForm::Full:
		case h_BlockForm::Unknown:
		case h_BlockForm::NumForms:
			H_ASSERT(false);
			break;

		case h_BlockForm::Plate:
			quad_count+= 2 * 2 + 6;
			break;

		case h_BlockForm::Bisected:
			// TODO
			break;
		};
	}

	return quad_count;
}

r_WorldVertex* r_ChunkInfo::BuildNonstandardFormBlocks( r_WorldVertex* v )
{
	//static const unsigned int c_next_index[8]= { 2, 2, 3, 4, 5, 6, 7, 8 };

	const h_World* world= chunk_->GetWorld();

	int X= chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	int Y= chunk_->Latitude () << H_CHUNK_WIDTH_LOG2;

	int relative_X= ( chunk_->Longitude() - world->Longitude() ) * H_CHUNK_WIDTH;
	int relative_Y= ( chunk_->Latitude () - world->Latitude () ) * H_CHUNK_WIDTH;

	bool flat_lighting= chunk_->IsEdgeChunk();

	const std::vector< h_NonstandardFormBlock* >& blocks= chunk_->GetNonstandartFormBlocksList();

	for( const h_NonstandardFormBlock* block : blocks )
	{
		switch( h_Block::Form(block->Type()) )
		{
		case h_BlockForm::Full:
		case h_BlockForm::Unknown:
		case h_BlockForm::NumForms:
			H_ASSERT(false);
			break;

		case h_BlockForm::Plate:
			{
				h_BlockType block_type= block->Type();
				int z= block->GetZ() << 1;

				int xy[8][2];
				xy[0][0]= 3 * ( X + block->GetX() );
				xy[0][1]= ( Y + block->GetY() ) * 2 - (1 & block->GetX()) + 2;

				xy[1][0]= xy[0][0] + 1;
				xy[1][1]= xy[0][1] + 1;
				xy[2][0]= xy[0][0] + 3;
				xy[2][1]= xy[0][1] + 1;
				xy[3][0]= xy[0][0] + 4;
				xy[3][1]= xy[0][1];
				xy[4][0]= xy[0][0] + 1;
				xy[4][1]= xy[0][1] - 1;
				xy[7][0]= xy[0][0] + 3;
				xy[7][1]= xy[0][1] - 1;

				unsigned char light[12][2];
				if( !flat_lighting )
				{
					int x= block->GetX() + relative_X;
					int y= block->GetY() + relative_Y;

					int block_z= block->GetZ();
					for( unsigned int i= 0; i < 2; i++ )
					{
						world->GetForwardVertexLight( x-1, y - (x&1)    , block_z+i, light[6*i+0] );
						world->GetBackVertexLight   ( x  , y + 1        , block_z+i, light[6*i+1] );
						world->GetForwardVertexLight( x  , y            , block_z+i, light[6*i+2] );
						world->GetBackVertexLight   ( x+1, y + ((1+x)&1), block_z+i, light[6*i+3] );
						world->GetForwardVertexLight( x  , y - 1        , block_z+i, light[6*i+4] );
						world->GetBackVertexLight   ( x  , y            , block_z+i, light[6*i+5] );
					}
					for( unsigned int i= 0; i < 6; i++ )
					{
						light[i+6][0]= (light[i][0] + light[i+6][0]) >> 1;
						light[i+6][1]= (light[i][1] + light[i+6][1]) >> 1;
					}
				}
				else
				{
					unsigned int addr= BlockAddr( block->GetX(), block->GetY(), block->GetZ() );
					light[0][0]= chunk_->GetSunLightData ()[ addr ];
					light[0][1]= chunk_->GetFireLightData()[ addr ];
					for( unsigned int i= 1; i < 12; i++ )
					{
						light[i][0]= light[0][0];
						light[i][1]= light[0][1];
					}
				}

				// Up and down sides
				for( unsigned int side= 0; side < 2; side++ )
				{
					for( unsigned int j= 0; j < 5; j++ )
					{
						v[j].coord[0]= xy[j][0];
						v[j].coord[1]= xy[j][1];
					}
					v[7].coord[0]= xy[7][0];
					v[7].coord[1]= xy[7][1];

					unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						side + (unsigned int)h_Direction::Up );

					if( r_TextureManager::TexturePerBlock( tex_id ) )
					{
						v[0].tex_coord[0]= 0;
						v[1].tex_coord[0]= v[4].tex_coord[0]= 1*H_TEXTURE_SCALE_MULTIPLIER;
						v[2].tex_coord[0]= v[7].tex_coord[0]= 3*H_TEXTURE_SCALE_MULTIPLIER;
						v[3].tex_coord[0]= 4*H_TEXTURE_SCALE_MULTIPLIER;

						v[0].tex_coord[1]= v[3].tex_coord[1]= 1*H_TEXTURE_SCALE_MULTIPLIER;
						v[1].tex_coord[1]= v[2].tex_coord[1]= 2*H_TEXTURE_SCALE_MULTIPLIER;
						v[7].tex_coord[1]= v[4].tex_coord[1]= 0;
					}
					else
					{
						unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

						v[0].tex_coord[0]= tex_scale * xy[0][0];
						v[1].tex_coord[0]= v[4].tex_coord[0]= v[0].tex_coord[0] + 1*tex_scale;
						v[2].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 3*tex_scale;
						v[3].tex_coord[0]= v[0].tex_coord[0] + 4*tex_scale;

						v[0].tex_coord[1]= v[3].tex_coord[1]= tex_scale * xy[0][0];
						v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + 1*tex_scale;
						v[7].tex_coord[1]= v[4].tex_coord[1]= v[0].tex_coord[1] - 1*tex_scale;
					}

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= v[7].tex_coord[2]= v[4].tex_coord[2]=
						tex_id;

					v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[7].coord[2]= v[4].coord[2]=
						z + 1 - side;

					for( unsigned int l= 0; l < 2; l++ )
					{
						unsigned int s= ( 1 - side ) * 6;
						v[0].light[l]= light[s+0][l];
						v[1].light[l]= light[s+1][l];
						v[2].light[l]= light[s+2][l];
						v[3].light[l]= light[s+3][l];
						v[7].light[l]= light[s+4][l];
						v[4].light[l]= light[s+5][l];
					}

					v[5]= v[0];
					v[6]= v[3];

					if( side == 1 )
					{
						std::swap( v[1], v[3] );
						std::swap( v[5], v[7] );
					}

					v+= 4 * 2;
				} // for up and down

				// forward and back part
				for( unsigned int side= 0; side < 2; side++ )
				{
					unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::Forward) + side );
					unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[0].coord[0]= v[ 1 ].coord[0]= 3 * ( block->GetX() + X ) + 1;
					v[0].coord[1]= v[ 1 ].coord[1]= v[2].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y - int(side) ) - (block->GetX()&1) + 2 + 1;

					v[0].coord[2]= v[ 3 ].coord[2]= z + 1;
					v[ 1 ].coord[2]= v[2].coord[2]= z;

					v[ 3 ].coord[0]= v[2].coord[0]= v[ 1 ].coord[0] + 2;

					v[0].tex_coord[0]= v[ 1 ].tex_coord[0]= v[0].coord[0] * tex_scale;
					v[2].tex_coord[0]= v[ 3 ].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;

					v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= z * tex_scale;
					v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					for( unsigned int l= 0; l < 2; l++ )
					{
						if( side == 0 )
						{
							v[0].light[l]= light[6+1][l];
							v[1].light[l]= light[0+1][l];
							v[3].light[l]= light[6+2][l];
							v[2].light[l]= light[0+2][l];
						}
						else
						{
							v[0].light[l]= light[6+5][l];
							v[1].light[l]= light[0+5][l];
							v[3].light[l]= light[6+4][l];
							v[2].light[l]= light[0+4][l];
						}
					}

					if( side == 1 )
						std::swap( v[1], v[3] );

					v+= 4;
				}

				// forward_right and back_left
				for( unsigned int side= 0; side < 2; side++ )
				{
					unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::ForwardRight) + side );
					unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(3*side);
					v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

					v[0].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - int(side);
					v[ 1 ].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;

					v[0].coord[2]= v[ 1 ].coord[2]= z + 1;
					v[2].coord[2]= v[ 3 ].coord[2]= z;

					v[ 1 ].tex_coord[0]= v[2].tex_coord[0]= tex_scale * ( v[ 1 ].coord[1] - v[1].coord[0] );
					v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[ 1 ].tex_coord[0] - 2 * tex_scale;

					v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * tex_scale;
					v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= v[0].tex_coord[1] - tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					for( unsigned int l= 0; l < 2; l++ )
					{
						if( side == 0 )
						{
							v[1].light[l]= light[6+2][l];
							v[2].light[l]= light[0+2][l];
							v[0].light[l]= light[6+3][l];
							v[3].light[l]= light[0+3][l];
						}
						else
						{
							v[1].light[l]= light[6+0][l];
							v[2].light[l]= light[0+0][l];
							v[0].light[l]= light[6+5][l];
							v[3].light[l]= light[0+5][l];
						}
					}

					if( side == 1 )
						std::swap( v[1], v[3] );

					v+=4;
				}

				// back_right and forward_left
				for( unsigned int side= 0; side < 2; side++ )
				{
					unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::ForwardLeft) + side );
					unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(side*3);
					v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

					v[ 1 ].coord[1]= v[2].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - 1 + int(side);
					v[0].coord[1]= v[ 3 ].coord[1]= v[ 1 ].coord[1] + 1;

					v[ 1 ].coord[2]= v[0].coord[2]= z + 1;
					v[2].coord[2]= v[ 3 ].coord[2]= z;

					v[2].tex_coord[0]= v[ 1 ].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;
					v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[2].tex_coord[0] + 2 * tex_scale;

					v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * tex_scale;
					v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					for( unsigned int l= 0; l < 2; l++ )
					{
						if( side == 1 )
						{
							v[0].light[l]= light[6+1][l];
							v[3].light[l]= light[0+1][l];
							v[1].light[l]= light[6+0][l];
							v[2].light[l]= light[0+0][l];
						}
						else
						{
							v[0].light[l]= light[6+3][l];
							v[3].light[l]= light[0+3][l];
							v[1].light[l]= light[6+4][l];
							v[2].light[l]= light[0+4][l];
						}
					}

					if( side == 0 )
						std::swap( v[1], v[3] );

					v+=4;
				}
			} // Plate
			break;

		case h_BlockForm::Bisected:
			// TODO
			break;
		};
	}

	return v;
}

void rBuildChunkFailingBlocks( const r_ChunkInfo& chunk_info, std::vector<r_WorldVertex>& out_vertices )
{
	const std::vector<h_FailingBlock*>& failing_blocks= chunk_info.chunk_->GetFailingBlocks();

	unsigned int vertex_count=
		failing_blocks.size() * (
			4 * 2 * 2 + // up/down
			4 * 2 + // forward/backward
			4 * 2 + // forward_right/back_left
			4 * 2 ); // forward_left/back_right

	out_vertices.resize( out_vertices.size() + vertex_count );
	r_WorldVertex* v= out_vertices.data() + out_vertices.size() - vertex_count;

	int X= chunk_info.chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	int Y= chunk_info.chunk_->Latitude () << H_CHUNK_WIDTH_LOG2;

	for( const h_FailingBlock* block : failing_blocks )
	{
		r_WorldVertex* v0= v;

		h_BlockType block_type= block->GetBlock()->Type();
		fixed8_t z= block->GetZ() >> 8;

		const short c_y_tex_coord= 64;

		// upper and lower part
		for( unsigned int side= 0; side < 2; side++ )
		{
			unsigned char tex_id=
				r_TextureManager::GetTextureId(
					block_type,
					static_cast<unsigned char>(h_Direction::Up) + side );
			unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

			v[0].coord[0]= 3 * ( block->GetX() + X );
			v[1].coord[0]= v[4].coord[0]= v[0].coord[0] + 1;
			v[2].coord[0]= v[7].coord[0]= v[0].coord[0] + 3;
			v[3].coord[0]= v[0].coord[0] + 4;

			v[0].coord[1]= v[3].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2;
			v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
			v[7].coord[1]= v[4].coord[1]= v[0].coord[1] - 1;

			v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[7].coord[2]= v[4].coord[2]= z + int((side^1) << 8);

			v[0].tex_coord[0]= tex_scale * v[0].coord[0];
			v[1].tex_coord[0]= v[4].tex_coord[0]= v[0].tex_coord[0] + 1*tex_scale;
			v[2].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 3*tex_scale;
			v[3].tex_coord[0]= v[0].tex_coord[0] + 4*tex_scale;

			v[0].tex_coord[1]= v[3].tex_coord[1]= tex_scale * v[0].coord[1];
			v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + 1*tex_scale;
			v[7].tex_coord[1]= v[4].tex_coord[1]= v[0].tex_coord[1] - 1*tex_scale;

			v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= v[7].tex_coord[2]= v[4].tex_coord[2]=
				tex_id;

			v[5]= v[0];
			v[6]= v[3];

			if( side == 1 )
			{
				std::swap( v[1], v[3] );
				std::swap( v[5], v[7] );
			}

			v+= 4 * 2;
		}

		// forward and back part
		for( unsigned int side= 0; side < 2; side++ )
		{
			unsigned char tex_id= r_TextureManager::GetTextureId(
				block_type,
				static_cast<unsigned char>(h_Direction::Forward) + side );
			unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

			v[0].coord[0]= v[ 1 ].coord[0]= 3 * ( block->GetX() + X ) + 1;
			v[0].coord[1]= v[ 1 ].coord[1]= v[2].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y - int(side) ) - (block->GetX()&1) + 2 + 1;

			v[0].coord[2]= v[ 3 ].coord[2]= z + (1<<8);
			v[ 1 ].coord[2]= v[2].coord[2]= z;

			v[ 3 ].coord[0]= v[2].coord[0]= v[ 1 ].coord[0] + 2;

			v[0].tex_coord[0]= v[ 1 ].tex_coord[0]= v[0].coord[0] * tex_scale;
			v[2].tex_coord[0]= v[ 3 ].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;

			v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= c_y_tex_coord;
			v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;

			v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

			if( side == 1 )
				std::swap( v[1], v[3] );

			v+= 4;
		}

		// forward_right and back_left
		for( unsigned int side= 0; side < 2; side++ )
		{
			unsigned char tex_id= r_TextureManager::GetTextureId(
				block_type,
				static_cast<unsigned char>(h_Direction::ForwardRight) + side );
			unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

			v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(3*side);
			v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

			v[0].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - int(side);
			v[ 1 ].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;

			v[0].coord[2]= v[ 1 ].coord[2]= z + (1 << 8);
			v[2].coord[2]= v[ 3 ].coord[2]= z;

			v[ 1 ].tex_coord[0]= v[2].tex_coord[0]= tex_scale * ( v[ 1 ].coord[1] - v[1].coord[0] );
			v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[ 1 ].tex_coord[0] - 2 * tex_scale;

			v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= c_y_tex_coord;
			v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;

			v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

			if( side == 1 )
				std::swap( v[1], v[3] );

			v+=4;
		}

		for( unsigned int side= 0; side < 2; side++ )
		{
			unsigned char tex_id= r_TextureManager::GetTextureId(
				block_type,
				static_cast<unsigned char>(h_Direction::ForwardLeft) + side );
			unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

			v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(side*3);
			v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

			v[ 1 ].coord[1]= v[2].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - 1 + int(side);
			v[0].coord[1]= v[ 3 ].coord[1]= v[ 1 ].coord[1] + 1;

			v[ 1 ].coord[2]= v[0].coord[2]= z + (1 << 8);
			v[2].coord[2]= v[ 3 ].coord[2]= z;

			v[2].tex_coord[0]= v[ 1 ].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;
			v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[2].tex_coord[0] + 2 * tex_scale;

			v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= c_y_tex_coord;
			v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;

			v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

			if( side == 0 )
				std::swap( v[1], v[3] );

			v+=4;
		}

		// Light are common for all block
		unsigned char light[2];
		unsigned int addr= BlockAddr( block->GetX(), block->GetY(), block->GetZ() >> 16 );
		light[0]= chunk_info.chunk_->GetSunLightData ()[ addr ] << 4;
		light[1]= chunk_info.chunk_->GetFireLightData()[ addr ] << 4;

		for( ; v0 < v; v0++ )
		{
			v0[0].light[0]= light[0];
			v0[0].light[1]= light[1];
		}
	} // for failing blocks in chunk
}
