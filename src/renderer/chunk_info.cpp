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

	const h_World& world= *chunk_->GetWorld();

	const int X= chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	const int Y= chunk_->Latitude()  << H_CHUNK_WIDTH_LOG2;
	const int chunk_loaded_zone_X= ( chunk_->Longitude() - world.Longitude() ) << H_CHUNK_WIDTH_LOG2;
	const int chunk_loaded_zone_Y= ( chunk_->Latitude () - world.Latitude () ) << H_CHUNK_WIDTH_LOG2;

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

				const int global_x= chunk_loaded_zone_X + b->x_;
				const int global_y= chunk_loaded_zone_Y + b->y_;
				const int forward_side_y= global_y + ( (global_x^1) & 1 );
				const int back_side_y= global_y - (global_x & 1);

				const int neighbors[6][2]=
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
					const int local_x= neighbors[d][0] & (H_CHUNK_WIDTH - 1);
					const int local_y= neighbors[d][1] & (H_CHUNK_WIDTH - 1);

					const h_Chunk* ch2=
						world.GetChunk(
							neighbors[d][0] >> H_CHUNK_WIDTH_LOG2,
							neighbors[d][1] >> H_CHUNK_WIDTH_LOG2 );

					unsigned int addr= BlockAddr( local_x, local_y, b->z_ );
					const h_Block* const b2= ch2->GetBlock( addr );
					const h_Block* const b3= ch2->GetBlock( addr+ 1 );

					static const unsigned int c_next_vi[6]= { 1, 2, 3, 4, 5, 0 };
					const unsigned int vi0= d;
					const unsigned int vi1= c_next_vi[d];

					if( b3->Type() == h_BlockType::Water )
						upper_block_is_water[vi0]= upper_block_is_water[vi1]= true;
					else if( b2->Type() == h_BlockType::Air )
						nearby_block_is_air[vi0]= nearby_block_is_air[vi1]= true;
					else if( b2->Type() == h_BlockType::Water )
					{
						const auto water_block= static_cast<const h_LiquidBlock*>(b2);

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

				world.GetForwardVertexLight( b->x_ + chunk_loaded_zone_X - 1, b->y_ + chunk_loaded_zone_Y - (b->x_&1), b->z_, v[0].light );
				world.GetBackVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y + 1, b->z_, v[1].light );
				world.GetForwardVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y, b->z_, v[2].light );
				world.GetBackVertexLight( b->x_ + chunk_loaded_zone_X + 1, b->y_ + chunk_loaded_zone_Y + ((1+b->x_)&1), b->z_, v[3].light );
				world.GetForwardVertexLight( b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y - 1, b->z_, v[4].light );
				world.GetBackVertexLight(  b->x_ + chunk_loaded_zone_X, b->y_ + chunk_loaded_zone_Y, b->z_, v[5].light );

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

				const int h=
					( b->z_ << R_WATER_VERTICES_Z_SCALER_LOG2 ) +
					( b->LiquidLevel() >> ( H_MAX_WATER_LEVEL_LOG2 - R_WATER_VERTICES_Z_SCALER_LOG2 ) );

				v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[4].coord[2]= v[5].coord[2]= short(h);

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
			const unsigned char t= t_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			const unsigned char t_fr= t_fr_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			const unsigned char t_br= t_br_p[z] & H_VISIBLY_TRANSPARENCY_BITS;
			const unsigned char t_up= t_p[z+1] & H_VISIBLY_TRANSPARENCY_BITS;
			const unsigned char t_f= t_f_p[z] & H_VISIBLY_TRANSPARENCY_BITS;

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

	const h_World& world= *chunk_->GetWorld();
	const int X= chunk_->Longitude() * H_CHUNK_WIDTH;
	const int Y= chunk_->Latitude () * H_CHUNK_WIDTH;
	const int relative_X= ( chunk_->Longitude() - world.Longitude() ) * H_CHUNK_WIDTH;
	const int relative_Y= ( chunk_->Latitude () - world.Latitude () ) * H_CHUNK_WIDTH;

	const bool flat_lighting= chunk_->IsEdgeChunk();

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
					world.GetForwardVertexLight( x + relative_X - 1, y + relative_Y - (x&1), z, v[0].light );
					world.GetBackVertexLight( x + relative_X, y + relative_Y + 1, z, v[1].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[2].light );
					world.GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((1+x)&1), z, v[3].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z, v[7].light );
					world.GetBackVertexLight(  x + relative_X, y + relative_Y, z, v[4].light );
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

				v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= (z * tex_scale) << 1;
				v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= v[2].tex_coord[1] + (tex_scale << 1);

				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;
				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					world.GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z, v[0].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[1].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y, z-1, v[2].light  );
					world.GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z-1, v[3].light  );
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

				v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= ( z * tex_scale ) << 1;
				v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= v[ 3 ].tex_coord[1] + (tex_scale << 1);

				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;
				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					world.GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z, v[0].light );
					world.GetBackVertexLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z - 1, v[3].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z - 1, v[2].light );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y - 1, z, v[1].light );
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

				v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= (z * tex_scale) << 1;
				v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= v[ 1 ].tex_coord[1] + (tex_scale << 1);

				v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=
				tex_id;

				if( flat_lighting )
				{
					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;
				}
				else
				{
					world.GetBackVertexLight( x + relative_X, y + relative_Y + 1, z, v[0].light );
					world.GetBackVertexLight( x + relative_X, y + relative_Y + 1, z - 1, v[1].light  );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y, z - 1, v[2].light  );
					world.GetForwardVertexLight( x + relative_X, y + relative_Y, z, v[3].light  );
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
			// 2 quads per upper and lower faces. 6 quads for sides.
			quad_count+= 2 * 2 + 6;
			break;

		case h_BlockForm::Bisected:
			quad_count+= 1 + 1 + 4;
			break;
		};
	}

	return quad_count;
}

r_WorldVertex* r_ChunkInfo::BuildNonstandardFormBlocks( r_WorldVertex* v )
{
	const h_World& world= *chunk_->GetWorld();

	const int X= chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	const int Y= chunk_->Latitude () << H_CHUNK_WIDTH_LOG2;

	const int relative_X= ( chunk_->Longitude() - world.Longitude() ) * H_CHUNK_WIDTH;
	const int relative_Y= ( chunk_->Latitude () - world.Latitude () ) * H_CHUNK_WIDTH;

	const bool flat_lighting= chunk_->IsEdgeChunk();

	const std::vector< h_NonstandardFormBlock* >& blocks= chunk_->GetNonstandartFormBlocksList();

	for( const h_NonstandardFormBlock* const block : blocks )
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
				if( block->Direction() == h_Direction::Down ) z++;

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
					const int x= block->GetX() + relative_X;
					const int y= block->GetY() + relative_Y;

					for( unsigned int i= 0; i < 2; i++ )
					{
						const int h= block->GetZ() - 1 + int(i);
						world.GetForwardVertexLight( x-1, y - (x&1)    , h, light[6*i+0] );
						world.GetBackVertexLight   ( x  , y + 1        , h, light[6*i+1] );
						world.GetForwardVertexLight( x  , y            , h, light[6*i+2] );
						world.GetBackVertexLight   ( x+1, y + ((1+x)&1), h, light[6*i+3] );
						world.GetForwardVertexLight( x  , y - 1        , h, light[6*i+4] );
						world.GetBackVertexLight   ( x  , y            , h, light[6*i+5] );
					}

					// Calculate light for vertices in block center.
					const unsigned int avg_index= block->Direction() == h_Direction::Up ? 6 : 0;
					for( unsigned int i= 0; i < 6; i++ )
					{
						light[i+avg_index][0]= (light[i][0] + light[i+6][0]) >> 1;
						light[i+avg_index][1]= (light[i][1] + light[i+6][1]) >> 1;
					}
				}
				else
				{
					const unsigned int addr= BlockAddr( block->GetX(), block->GetY(), block->GetZ() );
					light[0][0]= chunk_->GetSunLightData ()[ addr ] << 4;
					light[0][1]= chunk_->GetFireLightData()[ addr ] << 4;
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

					const unsigned char tex_id= r_TextureManager::GetTextureId(
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
						const unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

						v[0].tex_coord[0]= tex_scale * xy[0][0];
						v[1].tex_coord[0]= v[4].tex_coord[0]= v[0].tex_coord[0] + 1*tex_scale;
						v[2].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 3*tex_scale;
						v[3].tex_coord[0]= v[0].tex_coord[0] + 4*tex_scale;

						v[0].tex_coord[1]= v[3].tex_coord[1]= tex_scale * xy[0][1];
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

				for( unsigned int l= 0; l < 2; l++ )
				{
					v[0].light[l]= light[6+1][l];
					v[1].light[l]= light[0+1][l];
					v[3].light[l]= light[6+2][l];
					v[2].light[l]= light[0+2][l];

					v[4].light[l]= light[6+5][l];
					v[5].light[l]= light[0+5][l];
					v[7].light[l]= light[6+4][l];
					v[6].light[l]= light[0+4][l];
				}
				// forward and back part
				for( unsigned int side= 0; side < 2; side++ )
				{
					const unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::Forward) + side );
					const unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[0].coord[0]= v[ 1 ].coord[0]= 3 * ( block->GetX() + X ) + 1;
					v[0].coord[1]= v[ 1 ].coord[1]= v[2].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y - int(side) ) - (block->GetX()&1) + 2 + 1;

					v[0].coord[2]= v[ 3 ].coord[2]= z + 1;
					v[ 1 ].coord[2]= v[2].coord[2]= z;

					v[ 3 ].coord[0]= v[2].coord[0]= v[ 1 ].coord[0] + 2;

					v[0].tex_coord[0]= v[ 1 ].tex_coord[0]= v[0].coord[0] * tex_scale;
					v[2].tex_coord[0]= v[ 3 ].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;

					v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= z * tex_scale;
					v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= v[ 1 ].tex_coord[1] + tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					if( side == 1 )
						std::swap( v[1], v[3] );

					v+= 4;
				}

				for( unsigned int l= 0; l < 2; l++ )
				{
					v[1].light[l]= light[6+2][l];
					v[2].light[l]= light[0+2][l];
					v[0].light[l]= light[6+3][l];
					v[3].light[l]= light[0+3][l];

					v[5].light[l]= light[6+0][l];
					v[6].light[l]= light[0+0][l];
					v[4].light[l]= light[6+5][l];
					v[7].light[l]= light[0+5][l];
				}
				// forward_right and back_left
				for( unsigned int side= 0; side < 2; side++ )
				{
					const unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::ForwardRight) + side );
					const unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(3*side);
					v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

					v[0].coord[1]= v[ 3 ].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - int(side);
					v[ 1 ].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;

					v[0].coord[2]= v[ 1 ].coord[2]= z + 1;
					v[2].coord[2]= v[ 3 ].coord[2]= z;

					v[ 1 ].tex_coord[0]= v[2].tex_coord[0]= tex_scale * ( v[ 1 ].coord[1] - v[1].coord[0] );
					v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[ 1 ].tex_coord[0] - 2 * tex_scale;

					v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= z * tex_scale;
					v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= v[2].tex_coord[1] + tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					if( side == 1 )
						std::swap( v[1], v[3] );

					v+=4;
				}

				for( unsigned int l= 0; l < 2; l++ )
				{
					v[0].light[l]= light[6+3][l];
					v[3].light[l]= light[0+3][l];
					v[1].light[l]= light[6+4][l];
					v[2].light[l]= light[0+4][l];

					v[4].light[l]= light[6+1][l];
					v[7].light[l]= light[0+1][l];
					v[5].light[l]= light[6+0][l];
					v[6].light[l]= light[0+0][l];
				}
				// back_right and forward_left
				for( unsigned int side= 0; side < 2; side++ )
				{
					const unsigned char tex_id= r_TextureManager::GetTextureId(
						block_type,
						static_cast<unsigned char>(h_Direction::ForwardLeft) + side );
					const unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( block->GetX() + X ) + 3 - int(side*3);
					v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;

					v[ 1 ].coord[1]= v[2].coord[1]= 2 * ( block->GetY() + Y ) - (block->GetX()&1) + 2 - 1 + int(side);
					v[0].coord[1]= v[ 3 ].coord[1]= v[ 1 ].coord[1] + 1;

					v[ 1 ].coord[2]= v[0].coord[2]= z + 1;
					v[2].coord[2]= v[ 3 ].coord[2]= z;

					v[2].tex_coord[0]= v[ 1 ].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;
					v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[2].tex_coord[0] + 2 * tex_scale;

					v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= z * tex_scale;
					v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= v[ 3 ].tex_coord[1] + tex_scale;

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					if( side == 0 )
						std::swap( v[1], v[3] );

					v+=4;
				}
			} // Plate
			break;

		case h_BlockForm::Bisected:
			{
				static const unsigned int c_rot_table[]=
				{ 0, 1, 2, 3, 4, 5,  0, 1, 2, 3, 4, 5,  0, 1, 2, 3, 4, 5, };
				static const unsigned int c_dir_to_rot_table[6]=
				{ 0, 3,  1, 4,  5, 2 };
				const unsigned int rot= c_dir_to_rot_table[ ((unsigned int)block->Direction()) - ((unsigned int)h_Direction::Forward) ];

				const int z= block->GetZ() << 1;

				int xy[6][2];
				xy[0][0]= 3 * ( X + block->GetX() );
				xy[0][1]= ( Y + block->GetY() ) * 2 - (1 & block->GetX()) + 2;
				xy[1][0]= xy[0][0] + 1;
				xy[1][1]= xy[0][1] + 1;
				xy[2][0]= xy[0][0] + 3;
				xy[2][1]= xy[0][1] + 1;
				xy[3][0]= xy[0][0] + 4;
				xy[3][1]= xy[0][1];
				xy[4][0]= xy[0][0] + 3;
				xy[4][1]= xy[0][1] - 1;
				xy[5][0]= xy[0][0] + 1;
				xy[5][1]= xy[0][1] - 1;

				unsigned char light[12][2];
				if( !flat_lighting )
				{
					int x= block->GetX() + relative_X;
					int y= block->GetY() + relative_Y;
					for( unsigned int i= 0; i < 2; i++ )
					{
						int h= block->GetZ() - 1 + int(i);
						world.GetForwardVertexLight( x-1, y - (x&1)    , h, light[6*i+0] );
						world.GetBackVertexLight   ( x  , y + 1        , h, light[6*i+1] );
						world.GetForwardVertexLight( x  , y            , h, light[6*i+2] );
						world.GetBackVertexLight   ( x+1, y + ((1+x)&1), h, light[6*i+3] );
						world.GetForwardVertexLight( x  , y - 1        , h, light[6*i+4] );
						world.GetBackVertexLight   ( x  , y            , h, light[6*i+5] );
					}
				}
				else
				{
					const unsigned int addr= BlockAddr( block->GetX(), block->GetY(), block->GetZ() );
					light[0][0]= chunk_->GetSunLightData ()[ addr ] << 4;
					light[0][1]= chunk_->GetFireLightData()[ addr ] << 4;
					for( unsigned int i= 1; i < 12; i++ )
					{
						light[i][0]= light[0][0];
						light[i][1]= light[0][1];
					}
				}

				// Forward_left, forward, forward_right, back
				for( unsigned int side= 0; side < 4; side++ )
				{
					static const h_Direction c_dirs[4]= { h_Direction::ForwardLeft, h_Direction::Forward, h_Direction::ForwardRight, h_Direction::Back };
					unsigned char tex_id= r_TextureManager::GetTextureId( block->Type(), (unsigned char) c_dirs[side] );
					unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					unsigned int i0;
					unsigned int i1;
					if( side == 3 )
					{
						i0= c_rot_table[ rot + 3 ];
						i1= c_rot_table[ rot + 0 ];
					}
					else
					{
						i0= c_rot_table[ rot + side + 0 ];
						i1= c_rot_table[ rot + side + 1 ];
					}

					v[1].coord[0]= v[2].coord[0]= xy[i0][0];
					v[0].coord[0]= v[3].coord[0]= xy[i1][0];
					v[1].coord[1]= v[2].coord[1]= xy[i0][1];
					v[0].coord[1]= v[3].coord[1]= xy[i1][1];

					v[3].coord[2]= v[2].coord[2]= z;
					v[0].coord[2]= v[1].coord[2]= z + 2;

					if( v[0].coord[1] == v[2].coord[1] )
						for( unsigned int t= 0; t < 4; t++ ) v[t].tex_coord[0]= v[t].coord[0] * tex_scale;
					else
						for( unsigned int t= 0; t < 4; t++ ) v[t].tex_coord[0]= ( v[t].coord[1] * tex_scale ) << 1;
					v[3].tex_coord[1]= v[2].tex_coord[1]= z * tex_scale;
					v[0].tex_coord[1]= v[1].tex_coord[1]= z * tex_scale + (tex_scale<<1);
					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					for( unsigned int l= 0; l < 2; l++ )
					{
						v[0].light[l]= light[i1+6][l];
						v[1].light[l]= light[i0+6][l];
						v[2].light[l]= light[i0+0][l];
						v[3].light[l]= light[i1+0][l];
					}

					v+= 4;
				}

				// Up and down sides
				for( unsigned int side= 0; side < 2; side++ )
				{
					const unsigned char tex_id= r_TextureManager::GetTextureId(
						block->Type(),
						(unsigned int)h_Direction::Up + side )
						;
					const unsigned char tex_scale= r_TextureManager::GetTextureScale( tex_id );

					v[0].coord[0]= xy[ c_rot_table[rot+0] ][0];
					v[0].coord[1]= xy[ c_rot_table[rot+0] ][1];
					v[1].coord[0]= xy[ c_rot_table[rot+1] ][0];
					v[1].coord[1]= xy[ c_rot_table[rot+1] ][1];
					v[2].coord[0]= xy[ c_rot_table[rot+2] ][0];
					v[2].coord[1]= xy[ c_rot_table[rot+2] ][1];
					v[3].coord[0]= xy[ c_rot_table[rot+3] ][0];
					v[3].coord[1]= xy[ c_rot_table[rot+3] ][1];

					v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= z + ( (1-side) << 1 );

					for( unsigned int t= 0; t < 4; t++ )
					{
						v[t].tex_coord[0]= tex_scale * v[t].coord[0];
						v[t].tex_coord[1]= tex_scale * v[t].coord[01];
						v[t].tex_coord[2]= tex_id;
					}

					for( unsigned int l= 0; l < 2; l++ )
					{
						unsigned int s= 6 * ( 1 - side );
						v[0].light[l]= light[ s + c_rot_table[rot+0] ][l];
						v[1].light[l]= light[ s + c_rot_table[rot+1] ][l];
						v[2].light[l]= light[ s + c_rot_table[rot+2] ][l];
						v[3].light[l]= light[ s + c_rot_table[rot+3] ][l];
					}

					if( side == 1 )
						std::swap( v[1], v[3] );

					v+= 4;
				}
			} // h_BlockForm::Bisected
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

	const h_World& world= *chunk_info.chunk_->GetWorld();

	int X= chunk_info.chunk_->Longitude() << H_CHUNK_WIDTH_LOG2;
	int Y= chunk_info.chunk_->Latitude () << H_CHUNK_WIDTH_LOG2;
	int relative_X= ( chunk_info.chunk_->Longitude() - world.Longitude() ) << H_CHUNK_WIDTH_LOG2;
	int relative_Y= ( chunk_info.chunk_->Latitude () - world.Latitude () ) << H_CHUNK_WIDTH_LOG2;

	bool flat_lighting= chunk_info.chunk_->IsEdgeChunk();

	for( const h_FailingBlock* block : failing_blocks )
	{
		h_BlockType block_type= block->GetBlock()->Type();
		fixed8_t z= block->GetZ() >> 8;

		const short c_y_tex_coord= 64;

		unsigned char light[6 * 2][2];
		if( !flat_lighting )
		{
			unsigned char primary_light[6 * 3][2];

			int x= block->GetX() + relative_X;
			int y= block->GetY() + relative_Y;

			int int_z= block->GetZ() >> 16;
			for( unsigned int i= 0; i < 3; i++ )
			{
				int h= int_z - 1 + int(i);
				world.GetForwardVertexLight( x-1, y - (x&1)    , h, primary_light[6*i+0] );
				world.GetBackVertexLight   ( x  , y + 1        , h, primary_light[6*i+1] );
				world.GetForwardVertexLight( x  , y            , h, primary_light[6*i+2] );
				world.GetBackVertexLight   ( x+1, y + ((1+x)&1), h, primary_light[6*i+3] );
				world.GetForwardVertexLight( x  , y - 1        , h, primary_light[6*i+4] );
				world.GetBackVertexLight   ( x  , y            , h, primary_light[6*i+5] );
			}

			fixed8_t k= z & 255;
			fixed8_t one_minus_k= 256 - k;
			for( unsigned int h= 0; h < 12; h+= 6 )
			for( unsigned int s= 0; s < 6; s++ )
			for( unsigned int l= 0; l < 2; l++ )
				light[h+s][l]= ( primary_light[h+s][l] * one_minus_k + primary_light[h+6+s][l] * k ) >> 8;
		}
		else
		{
			unsigned int addr= BlockAddr( block->GetX(), block->GetY(), block->GetZ() >> 16 );
			unsigned char l[2];
			l[0]= chunk_info.chunk_->GetSunLightData ()[ addr ] << 4;
			l[1]= chunk_info.chunk_->GetFireLightData()[ addr ] << 4;
			for( unsigned int i= 0; i < 12; i++ )
			{
				light[i][0]= l[0];
				light[i][1]= l[1];
			}
		}


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

			for( unsigned int l= 0; l < 2; l++ )
			{
				unsigned int h= (1 - side) * 6;
				v[0].light[l]= light[h+0][l];
				v[1].light[l]= light[h+1][l];
				v[2].light[l]= light[h+2][l];
				v[3].light[l]= light[h+3][l];
				v[7].light[l]= light[h+4][l];
				v[4].light[l]= light[h+5][l];
			}

			v[5]= v[0];
			v[6]= v[3];

			if( side == 1 )
			{
				std::swap( v[1], v[3] );
				std::swap( v[5], v[7] );
			}

			v+= 4 * 2;
		}

		for( unsigned int l= 0; l < 2; l++ )
		{
			v[0].light[l]= light[6+1][l];
			v[1].light[l]= light[0+1][l];
			v[2].light[l]= light[0+2][l];
			v[3].light[l]= light[6+2][l];

			v[4+0].light[l]= light[6+5][l];
			v[4+1].light[l]= light[0+5][l];
			v[4+2].light[l]= light[0+4][l];
			v[4+3].light[l]= light[6+4][l];
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

		for( unsigned int l= 0; l < 2; l++ )
		{
			v[0].light[l]= light[6+3][l];
			v[1].light[l]= light[6+2][l];
			v[2].light[l]= light[0+2][l];
			v[3].light[l]= light[0+3][l];

			v[4+0].light[l]= light[6+5][l];
			v[4+1].light[l]= light[6+0][l];
			v[4+2].light[l]= light[0+0][l];
			v[4+3].light[l]= light[0+5][l];
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

		for( unsigned int l= 0; l < 2; l++ )
		{
			v[0].light[l]= light[6+3][l];
			v[1].light[l]= light[6+4][l];
			v[2].light[l]= light[0+4][l];
			v[3].light[l]= light[0+3][l];

			v[4+0].light[l]= light[6+1][l];
			v[4+1].light[l]= light[6+0][l];
			v[4+2].light[l]= light[0+0][l];
			v[4+3].light[l]= light[0+1][l];
		}
		// forward_left and back_right
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
	} // for failing blocks in chunk
}

void r_ChunkInfo::GetQuadCountLowDetail()
{
	// TODO
	min_geometry_height_= 1;
	max_geometry_height_= H_CHUNK_HEIGHT - 1;

	vertex_count_= 8192 * 4;
}

void r_ChunkInfo::BuildChunkMeshLowDetail()
{
/*
 __    __
/ Ä\__/ @\__
\__/ @\__/ &\
/ d\__/ d\__/
\__/ d\__/ =\
/ d\__/ d\__/
\__/ d\__/ =\
/ Ü\__/ *\__/
\__/ *\__/ +\
   \__/  \__/

*/
	r_WorldVertex* v= vertex_data_;

	const h_World& world= *chunk_->GetWorld();
	const int X= chunk_->Longitude() * H_CHUNK_WIDTH;
	const int Y= chunk_->Latitude () * H_CHUNK_WIDTH;
	const int relative_X= ( chunk_->Longitude() - world.Longitude() ) * H_CHUNK_WIDTH;
	const int relative_Y= ( chunk_->Latitude () - world.Latitude () ) * H_CHUNK_WIDTH;

	unsigned int quad_count= 0u;

	for( int x= 0; x< H_CHUNK_WIDTH; x++ )
	for( int y= 0; y< H_CHUNK_WIDTH; y++ )
	{
		int offset;

		const unsigned char* t_p [7];
		const h_Block* const* b_p[7];
		const unsigned char* ls_p[7];
		const unsigned char* lf_p[7];

		offset= BlockAddr(x,y,0); // BLock itself
		t_p [6]= chunk_->GetTransparencyData() + offset;
		b_p [6]= chunk_->GetBlocksData() + offset;
		ls_p[6]= chunk_->GetSunLightData() + offset;
		lf_p[6]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x,y+1,0); // forward
		t_p [0]= chunk_->GetTransparencyData() + offset;
		b_p [0]= chunk_->GetBlocksData() + offset;
		ls_p[0]= chunk_->GetSunLightData() + offset;
		lf_p[0]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x + 1, y + (1&(x+1)),0); // forward right
		t_p [1]= chunk_->GetTransparencyData() + offset;
		b_p [1]= chunk_->GetBlocksData() + offset;
		ls_p[1]= chunk_->GetSunLightData() + offset;
		lf_p[1]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x + 1, y - ( 1&x )  ,0); // back right
		t_p [2]= chunk_->GetTransparencyData() + offset;
		b_p [2]= chunk_->GetBlocksData() + offset;
		ls_p[2]= chunk_->GetSunLightData() + offset;
		lf_p[2]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x,y-1,0); // back
		t_p [3]= chunk_->GetTransparencyData() + offset;
		b_p [3]= chunk_->GetBlocksData() + offset;
		ls_p[3]= chunk_->GetSunLightData() + offset;
		lf_p[3]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x - 1, y - ( 1&x )  ,0); // back left
		t_p [4]= chunk_->GetTransparencyData() + offset;
		b_p [4]= chunk_->GetBlocksData() + offset;
		ls_p[4]= chunk_->GetSunLightData() + offset;
		lf_p[4]= chunk_->GetFireLightData() + offset;

		offset= BlockAddr(x - 1, y + (1&(x+1)),0); // forward left
		t_p [5]= chunk_->GetTransparencyData() + offset;
		b_p [5]= chunk_->GetBlocksData() + offset;
		ls_p[5]= chunk_->GetSunLightData() + offset;
		lf_p[5]= chunk_->GetFireLightData() + offset;

		// TODO - something is wrong here. Check it twice.

		//front chunk border
		if( y == H_CHUNK_WIDTH - 1 && x > 0 && x < H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
			{
				offset= BlockAddr( x, 0, 0 );
				t_p [0]= chunk_front_->GetTransparencyData() + offset;
				b_p [0]= chunk_front_->GetBlocksData() + offset;
				ls_p[0]= chunk_front_->GetSunLightData() + offset;
				lf_p[0]= chunk_front_->GetFireLightData() + offset;
			}
			else
				t_p[0]= t_p[6];//this block transparency

			if( (x&1) == 0 )
			{
				if( chunk_front_ != nullptr )
				{
					offset= BlockAddr( x + 1, 0, 0 );
					t_p [1]= chunk_front_->GetTransparencyData() + offset;
					b_p [1]= chunk_front_->GetBlocksData() + offset;
					ls_p[1]= chunk_front_->GetSunLightData() + offset;
					lf_p[1]= chunk_front_->GetFireLightData() + offset;

					offset= BlockAddr( x - 1, 0, 0 );
					t_p [5]= chunk_front_->GetTransparencyData() + offset;
					b_p [5]= chunk_front_->GetBlocksData() + offset;
					ls_p[5]= chunk_front_->GetSunLightData() + offset;
					lf_p[5]= chunk_front_->GetFireLightData() + offset;
				}
				else
					t_p[1]= t_p[5]= t_p[6];//this block transparency
			}
		}
		//back chunk border
		else if( y == 0 && x > 0 && x < H_CHUNK_WIDTH - 1 )
		{
			if( chunk_back_ != nullptr )
			{
				offset= BlockAddr( x, H_CHUNK_WIDTH - 1, 0 );
				t_p [3]= chunk_back_->GetTransparencyData() + offset;
				b_p [3]= chunk_back_->GetBlocksData() + offset;
				ls_p[3]= chunk_back_->GetSunLightData() + offset;
				lf_p[3]= chunk_back_->GetFireLightData() + offset;
			}
			else
				t_p[3]= t_p[6];//this block transparency

			if( (x&1) != 0 )
			{
				if( chunk_back_ != nullptr )
				{
					offset= BlockAddr( x+ 1, H_CHUNK_WIDTH - 1, 0 );
					t_p [2]= chunk_back_->GetTransparencyData() + offset;
					b_p [2]= chunk_back_->GetBlocksData() + offset;
					ls_p[2]= chunk_back_->GetSunLightData() + offset;
					lf_p[2]= chunk_back_->GetFireLightData() + offset;

					offset= BlockAddr( x- 1, H_CHUNK_WIDTH - 1, 0 );
					t_p [4]= chunk_back_->GetTransparencyData() + offset;
					b_p [4]= chunk_back_->GetBlocksData() + offset;
					ls_p[4]= chunk_back_->GetSunLightData() + offset;
					lf_p[4]= chunk_back_->GetFireLightData() + offset;
				}
				else
					t_p[2]= t_p[4]= t_p[6];//this block transparency
			}
		}
		//right chunk border
		else if( x == H_CHUNK_WIDTH - 1 && y > 0 && y < H_CHUNK_WIDTH-1 )
		{
			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, y, 0 );
				t_p [1]= chunk_right_->GetTransparencyData() + offset;
				b_p [1]= chunk_right_->GetBlocksData() + offset;
				ls_p[1]= chunk_right_->GetSunLightData() + offset;
				lf_p[1]= chunk_right_->GetFireLightData() + offset;

				offset= BlockAddr( 0, y - 1, 0 );
				t_p [2]= chunk_right_->GetTransparencyData() + offset;
				b_p [2]= chunk_right_->GetBlocksData() + offset;
				ls_p[2]= chunk_right_->GetSunLightData() + offset;
				lf_p[2]= chunk_right_->GetFireLightData() + offset;
			}
			else
				t_p[1]= t_p[2]= t_p[6];//this block transparency
		}
		// left chunk border
		else if( x == 0 && y > 0 && y < H_CHUNK_WIDTH - 1 )
		{
			if( chunk_left_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, y, 0 );
				t_p [4]= chunk_left_->GetTransparencyData() + offset;
				b_p [4]= chunk_left_->GetBlocksData() + offset;
				ls_p[4]= chunk_left_->GetSunLightData() + offset;
				lf_p[4]= chunk_left_->GetFireLightData() + offset;

				offset= BlockAddr( H_CHUNK_WIDTH - 1, y + 1, 0 );
				t_p [5]= chunk_left_->GetTransparencyData() + offset;
				b_p [5]= chunk_left_->GetBlocksData() + offset;
				ls_p[5]= chunk_left_->GetSunLightData() + offset;
				lf_p[5]= chunk_left_->GetFireLightData() + offset;
			}
			else
				t_p[4]= t_p[5]= t_p[6];//this block transparency
		}
		// right front corner
		else if( x == H_CHUNK_WIDTH - 1 && y == H_CHUNK_WIDTH - 1 )
		{
			if( chunk_front_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, 0, 0 );
				t_p [0]= chunk_front_->GetTransparencyData() + offset;
				b_p [0]= chunk_front_->GetBlocksData() + offset;
				ls_p[0]= chunk_front_->GetSunLightData() + offset;
				lf_p[0]= chunk_front_->GetFireLightData() + offset;
			}
			else
				t_p[0]= t_p[6];//this block transparency

			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, H_CHUNK_WIDTH - 1, 0 );
				t_p [1]= chunk_right_->GetTransparencyData() + offset;
				b_p [1]= chunk_right_->GetBlocksData() + offset;
				ls_p[1]= chunk_right_->GetSunLightData() + offset;
				lf_p[1]= chunk_right_->GetFireLightData() + offset;

				offset= BlockAddr( 0, H_CHUNK_WIDTH - 2, 0 );
				t_p [2]= chunk_right_->GetTransparencyData() + offset;
				b_p [2]= chunk_right_->GetBlocksData() + offset;
				ls_p[2]= chunk_right_->GetSunLightData() + offset;
				lf_p[2]= chunk_right_->GetFireLightData() + offset;
			}
			else
				t_p[1]= t_p[2]= t_p[6];//this block transparency
		}
		// right back corner
		else if( x == H_CHUNK_WIDTH - 1 && y == 0 )
		{
			if( chunk_right_ != nullptr )
			{
				offset= BlockAddr( 0, 0, 0 );
				t_p [1]= chunk_right_->GetTransparencyData() + offset;
				b_p [1]= chunk_right_->GetBlocksData() + offset;
				ls_p[1]= chunk_right_->GetSunLightData() + offset;
				lf_p[1]= chunk_right_->GetFireLightData() + offset;
			}
			else
				t_p[1]= t_p[6];//this block transparency;
			if( chunk_back_right_ != nullptr )
			{
				offset= BlockAddr( 0, H_CHUNK_WIDTH - 1, 0 );
				t_p [2]= chunk_back_right_->GetTransparencyData() + offset;
				b_p [2]= chunk_back_right_->GetBlocksData() + offset;
				ls_p[2]= chunk_back_right_->GetSunLightData() + offset;
				lf_p[2]= chunk_back_right_->GetFireLightData() + offset;
			}
			else
				t_p[2]= t_p[6];//this block transparency;
			if( chunk_back_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, H_CHUNK_WIDTH - 1, 0 );
				t_p [3]= chunk_back_->GetTransparencyData() + offset;
				b_p [3]= chunk_back_->GetBlocksData() + offset;
				ls_p[3]= chunk_back_->GetSunLightData() + offset;
				lf_p[3]= chunk_back_->GetFireLightData() + offset;

				offset= BlockAddr( H_CHUNK_WIDTH - 2, H_CHUNK_WIDTH - 1, 0 );
				t_p [4]= chunk_back_->GetTransparencyData() + offset;
				b_p [4]= chunk_back_->GetBlocksData() + offset;
				ls_p[4]= chunk_back_->GetSunLightData() + offset;
				lf_p[4]= chunk_back_->GetFireLightData() + offset;
			}
			else
				t_p[3]= t_p[4]= t_p[6];//this block transparency;
		}
		// left front corner
		else if( x == 0 && y == H_CHUNK_WIDTH - 1 )
		{
			if( chunk_left_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, H_CHUNK_WIDTH - 1, 0 );
				t_p [4]= chunk_left_->GetTransparencyData() + offset;
				b_p [4]= chunk_left_->GetBlocksData() + offset;
				ls_p[4]= chunk_left_->GetSunLightData() + offset;
				lf_p[4]= chunk_left_->GetFireLightData() + offset;
			}
			else
				t_p[4]= t_p[6];//this block transparency

			if( chunk_front_left_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, 0, 0 );
				t_p [5]= chunk_front_left_->GetTransparencyData() + offset;
				b_p [5]= chunk_front_left_->GetBlocksData() + offset;
				ls_p[5]= chunk_front_left_->GetSunLightData() + offset;
				lf_p[5]= chunk_front_left_->GetFireLightData() + offset;
			}
			else
				t_p[5]= t_p[6];//this block transparency
		}
		// left back corner
		else if( x == 0 && y == 0 )
		{
			if( chunk_left_ != nullptr )
			{
				offset= BlockAddr( H_CHUNK_WIDTH - 1, 0, 0 );
				t_p [4]= chunk_left_->GetTransparencyData() + offset;
				b_p [4]= chunk_left_->GetBlocksData() + offset;
				ls_p[4]= chunk_left_->GetSunLightData() + offset;
				lf_p[4]= chunk_left_->GetFireLightData() + offset;

				offset= BlockAddr( H_CHUNK_WIDTH - 1, 1, 0 );
				t_p [5]= chunk_left_->GetTransparencyData() + offset;
				b_p [5]= chunk_left_->GetBlocksData() + offset;
				ls_p[5]= chunk_left_->GetSunLightData() + offset;
				lf_p[5]= chunk_left_->GetFireLightData() + offset;
			}
			else
				t_p[4]= t_p[5]= t_p[6];//this block transparency

			if( chunk_back_ != nullptr )
			{
				offset= BlockAddr( 0, H_CHUNK_WIDTH - 1, 0 );
				t_p [3]= chunk_back_->GetTransparencyData() + offset;
				b_p [3]= chunk_back_->GetBlocksData() + offset;
				ls_p[3]= chunk_back_->GetSunLightData() + offset;
				lf_p[3]= chunk_back_->GetFireLightData() + offset;
			}
			else
				t_p[3]= t_p[6];//this block transparency
		}

		for( int z= min_geometry_height_; z<= max_geometry_height_; z++ )
		{
			unsigned char tex_id, tex_scale, light[2], normal_id;

			unsigned char t= t_p[6][z] & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_up= t_p[6][z+1]  & H_VISIBLY_TRANSPARENCY_BITS;
			unsigned char t_down= t_p[6][z+1]  & H_VISIBLY_TRANSPARENCY_BITS;
			const h_Block* const b= b_p[6][z];

			bool have_up_face= false;
			bool have_down_face= false;
			bool have_sides[6]= { false, false, false, false, false, false };
			bool have_any_side= false;
			if( t < t_up ) have_up_face= true;
			if( t < t_down ) have_down_face= true;
			for( unsigned int side= 0u; side < 6u; ++side )
			{
				have_sides[side]= t < ( t_p[side][z] & H_VISIBLY_TRANSPARENCY_BITS );
				if( have_sides[side] ) have_any_side= true;
			}

			r_WorldVertex cv[6u];
			cv[0].coord[0]= cv[4].coord[0]= 3 * ( x + X ) + 1;
			cv[1].coord[0]= cv[3].coord[0]= cv[0].coord[0] + 2;
			cv[2].coord[0]= cv[0].coord[0] + 3;
			cv[5].coord[0]= cv[0].coord[0] - 1;

			cv[0].coord[1]= cv[1].coord[1]= 2 * ( y + Y ) - (x&1) + 3;
			cv[2].coord[1]= cv[5].coord[1]= cv[0].coord[1] - 1;
			cv[3].coord[1]= cv[4].coord[1]= cv[0].coord[1] - 2;


			if( have_any_side )
			{
				unsigned int first_existent_side_index= 0u;
				for( unsigned int side= 0u; side < 6u; ++side )
					if( !have_sides[side] && have_sides[(side + 1u)% 6u] )
					{
						first_existent_side_index= ( side + 1u )% 6u;
						break;
					}

				for( unsigned int side= first_existent_side_index; side < first_existent_side_index + 6u; )
				{
					unsigned int side_b= side % 6u;
					if( !have_sides[side_b] )
					{
						++side;
						continue;
					}
					unsigned int side_b_next= ( side + 1u ) % 6u;
					unsigned int side_b_next_next= ( side + 2u ) % 6u;

					normal_id= static_cast<unsigned char>(h_Direction::Forward);
					tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
					tex_scale= r_TextureManager::GetTextureScale( tex_id );

					const r_WorldVertex* quad_v0;
					const r_WorldVertex* quad_v1;
					if( have_sides[side_b_next] )
					{
						light[0]= ( ls_p[side_b][z] + ls_p[side_b_next][z] ) << 3;
						light[1]= ( lf_p[side_b][z] + lf_p[side_b_next][z] ) << 3;
						quad_v0= &cv[side_b];
						quad_v1= & cv[side_b_next_next];
						side+= 2u;
					}
					else
					{
						light[0]= ls_p[side_b][z] << 4;
						light[1]= lf_p[side_b][z] << 4;
						quad_v0= &cv[side_b];
						quad_v1= & cv[side_b_next];
						++side;
					}

					v[0]= v[1]= *quad_v0;
					v[2]= v[3]= *quad_v1;

					v[0].coord[2]= v[3].coord[2]= (z+1) << 1;
					v[1].coord[2]= v[2].coord[2]= z << 1;

					v[0].tex_coord[0]= v[1].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;
					v[2].tex_coord[0]= v[3].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;

					v[0].tex_coord[1]= v[3].tex_coord[1]= ( z * tex_scale ) << 1;
					v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + (tex_scale << 1);

					v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= tex_id;

					v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0];
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1];

					v+= 4u;
					++quad_count;
					if( quad_count * 4u >= vertex_count_ )
						return;
				} // for sides
			} // if have any side

			for( unsigned int up_down= 0; up_down < 2; ++up_down )
			{
				if( up_down == 1 && !have_up_face )
					continue;
				else if( !have_down_face )
					continue;

				normal_id= static_cast<unsigned char>(h_Direction::Up);
				tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );
				int light_z= up_down == 0 ? (z-1) : (z+1);
				light[0]= ls_p[6][light_z] << 4;
				light[1]= lf_p[6][light_z] << 4;
				tex_scale= r_TextureManager::GetTextureScale( tex_id );

				v[0]= cv[5];
				v[1]= cv[0];
				v[2]= cv[1];
				v[3]= cv[2];
				v[4]= cv[4];
				v[7]= cv[3];

				for( unsigned int vn= 0; vn < 8; ++vn )
				{
					v[vn].coord[2]= ( z + up_down ) << 1;
					v[vn].light[0]= light[0];
					v[vn].light[1]= light[1];
					v[vn].tex_coord[2]= tex_id;
				}

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

				v[5]= v[0];
				v[6]= v[3];

				if( up_down == 0 )
				{
					std::swap( v[1], v[3] );
					std::swap( v[5], v[7] );
				}
				v+= 8;
				quad_count+= 2u;
				if( quad_count * 4u >= vertex_count_ )
					return;
			}

		} // for z
	} // for xy

	vertex_count_= quad_count * 4u;
}
