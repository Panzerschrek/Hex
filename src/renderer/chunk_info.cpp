#ifndef CHUNK_INFO_CPP
#define CHUNK_INFO_CPP

#include "renderer.hpp"
#include "texture_manager.hpp"



void r_WaterQuadChunkInfo::GetVertexCount()
{
    new_vertex_count= 0;
    for( unsigned int i= 0; i< 2; i++ )
        for( unsigned int j= 0; j< 2; j++ )
        {
            if( chunks[i][j] != NULL )
                new_vertex_count+=  chunks[i][j]->water_surface_mesh_vertices.Size();
        }
}

void r_WaterQuadChunkInfo::GetUpdatedState()
{
    water_updated= false;
    for( unsigned int i= 0; i< 2; i++ )
        for( unsigned int j= 0; j< 2; j++ )
            if( chunks[i][j] != NULL )
                water_updated= water_updated || chunks[i][j]->chunk_water_data_updated;
}

void r_WaterQuadChunkInfo::BuildFinalMesh()
{
    r_WaterVertex* v= vb_data;
    unsigned int s;

    for( unsigned int i= 0; i< 2; i++ )
        for( unsigned int j= 0; j< 2; j++ )
        {
            if( chunks[i][j] != NULL )
            {
                s= chunks[i][j]->water_surface_mesh_vertices.Size();
                memcpy( v, chunks[i][j]->water_surface_mesh_vertices.Data(), s * sizeof( r_WaterVertex ) );
                v+= s;
            }
        }

    real_vertex_count= new_vertex_count;
}


r_ChunkInfo::r_ChunkInfo()
{
    chunk_front= chunk_right= chunk_back_right= chunk_back= NULL;
    chunk_mesh_rebuilded= chunk_data_updated= false;
}

void r_ChunkInfo::BuildWaterSurfaceMesh()
{
    auto water_block_list= chunk->GetWaterList();
    m_Collection< h_LiquidBlock* >::ConstIterator iter( water_block_list );
    const h_LiquidBlock* b;
    r_WaterVertex* v, *sv;
    short h;

    water_surface_mesh_vertices.Resize(0);
    water_side_mesh_vertices.Resize(0);

    short X= chunk->Longitude() * H_CHUNK_WIDTH;
    short Y= chunk->Latitude() * H_CHUNK_WIDTH;
    short chunk_loaded_zone_X;
    short chunk_loaded_zone_Y;


    unsigned int vertex_water_level[6];
    unsigned short vertex_water_block_count[6];
    h_World* world= chunk->GetWorld();
    short global_x, global_y;
    short nearby_block_x, nearby_block_y;


    chunk_loaded_zone_X= ( chunk->Longitude() - world->Longitude() ) * H_CHUNK_WIDTH;
    chunk_loaded_zone_Y= ( chunk->Latitude() - world->Latitude() ) * H_CHUNK_WIDTH;
    if( ! chunk->IsEdgeChunk() )
    {
        for( iter.Begin(); iter.IsValid(); iter.Next() )
        {
            b= *iter;

            h_BlockType type= chunk->GetBlock( b->x, b->y, b->z + 1 )->Type();
            if( type  == AIR  || (  b->LiquidLevel() < H_MAX_WATER_LEVEL && type != WATER ) )
            {
                // water_surface_mesh_vertices.Resize( water_surface_mesh_vertices.Size() + 6 );
                water_surface_mesh_vertices.AddToSize(6);
                v= water_surface_mesh_vertices.Last() - 5;//water_surface_mesh_vertices.Data() + water_surface_mesh_vertices.Size() - 6;
                v[0].coord[0]= 3 * ( b->x + X );
                v[1].coord[0]= v[5].coord[0]= v[0].coord[0] + 1;
                v[2].coord[0]= v[4].coord[0]= v[0].coord[0] + 3;
                v[3].coord[0]= v[0].coord[0] + 4;

                v[0].coord[1]= v[3].coord[1]= 2 * ( b->y + Y ) - ((b->x)&1) + 2;
                v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
                v[4].coord[1]= v[5].coord[1]= v[0].coord[1] - 1;


                //calculate height of vertices
                h_Block* b2;
                vertex_water_level[0]= vertex_water_level[1]= vertex_water_level[2]=
                                           vertex_water_level[3]= vertex_water_level[4]= vertex_water_level[5]= b->LiquidLevel();
                vertex_water_block_count[0]= vertex_water_block_count[1]= vertex_water_block_count[2]=
                                                 vertex_water_block_count[3]= vertex_water_block_count[4]= vertex_water_block_count[5]= 1;

                //forward
                global_x= b->x + chunk_loaded_zone_X;
                global_y= b->y + chunk_loaded_zone_Y + 1;
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[1]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[2]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[1]++;
                    vertex_water_block_count[2]++;
                }
                /*else if( b2->Type() == AIR )
                {
                	water_side_mesh_vertices.Resize( water_side_mesh_vertices.Size() + 4 );
                	sv= water_side_mesh_vertices.Data() + water_side_mesh_vertices.Size() - 4;
                	sv[0].coord[0]= sv[1].coord[0]= v[1].coord[0];
                	sv[2].coord[0]= sv[3].coord[0]= sv[0].coord[0]+ 2;//x
                	sv[0].coord[1]= sv[1].coord[1]= sv[2].coord[1]= sv[3].coord[1]=  v[1].coord[1];//y
                	sv[0].coord[2]= sv[3].coord[2]= (b->z)<<7;
                	sv[1].coord[2]= sv[2].coord[2]= sv[0].coord[2] - (1<<7);//z
                }*/

                //back
                global_x= b->x + chunk_loaded_zone_X;
                global_y= b->y + chunk_loaded_zone_Y - 1;
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[4]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[5]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[4]++;
                    vertex_water_block_count[5]++;
                }
                else if( b2->Type() == AIR )
                {
                }
                //forward right
                global_x= b->x + chunk_loaded_zone_X + 1;
                global_y= b->y + chunk_loaded_zone_Y +((b->x+1)&1);
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[2]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[3]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[2]++;
                    vertex_water_block_count[3]++;
                }
                else if( b2->Type() == AIR )
                {
                }

                //back left
                global_x= b->x + chunk_loaded_zone_X - 1;
                global_y= b->y + chunk_loaded_zone_Y - (b->x&1);
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[0]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[5]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[0]++;
                    vertex_water_block_count[5]++;
                }
                else if( b2->Type() == AIR )
                {
                }

                //back right
                global_x= b->x + chunk_loaded_zone_X + 1;
                global_y= b->y + chunk_loaded_zone_Y -(b->x&1);
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[3]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[4]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[3]++;
                    vertex_water_block_count[4]++;
                }
                else if( b2->Type() == AIR )
                {
                }

                //forward left
                global_x= b->x + chunk_loaded_zone_X - 1;
                global_y= b->y + chunk_loaded_zone_Y + ((b->x+1)&1);
                nearby_block_x= (global_x)&( H_CHUNK_WIDTH-1);
                nearby_block_y= (global_y)&( H_CHUNK_WIDTH-1);
                b2=world->GetChunk( global_x >> H_CHUNK_WIDTH_LOG2,
                                    global_y >> H_CHUNK_WIDTH_LOG2 )
                   ->GetBlock( nearby_block_x, nearby_block_y, b->z );
                if( b2->Type() == WATER )
                {
                    vertex_water_level[0]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_level[1]+= ((h_LiquidBlock*)b2)->LiquidLevel();
                    vertex_water_block_count[0]++;
                    vertex_water_block_count[1]++;
                }
                else if( b2->Type() == AIR )
                {
                }

                for( unsigned int k= 0; k< 6; k++ )
                    v[k].coord[2]= ((b->z-1)<<7) +  vertex_water_level[k] / ( vertex_water_block_count[k] * ( H_MAX_WATER_LEVEL / 128) );
                v[0].light[0]= world->GetForwardVertexSunLight( b->x + chunk_loaded_zone_X - 1, b->y + chunk_loaded_zone_Y - (b->x&1), b->z );
                v[1].light[0]= world->GetBackVertexSunLight( b->x + chunk_loaded_zone_X, b->y + chunk_loaded_zone_Y + 1, b->z );
                v[2].light[0]= world->GetForwardVertexSunLight( b->x + chunk_loaded_zone_X, b->y + chunk_loaded_zone_Y, b->z );
                v[3].light[0]= world->GetBackVertexSunLight( b->x + chunk_loaded_zone_X + 1, b->y + chunk_loaded_zone_Y + ((1+b->x)&1), b->z );
                v[4].light[0]= world->GetForwardVertexSunLight( b->x + chunk_loaded_zone_X, b->y + chunk_loaded_zone_Y - 1, b->z );
                v[5].light[0]= world->GetBackVertexSunLight(  b->x + chunk_loaded_zone_X, b->y + chunk_loaded_zone_Y, b->z );

                v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= v[4].light[1]= v[5].light[1]=
                chunk->FireLightLevel( b->x, b->y, b->z + 1 ) << 4;


            }
        }

    }//smooth water surface
    else
    {
        for( iter.Begin(); iter.IsValid(); iter.Next() )
        {
            b= *iter;

            h_BlockType type= chunk->GetBlock( b->x, b->y, b->z + 1 )->Type();
            if( type  == AIR  || (  b->LiquidLevel() < H_MAX_WATER_LEVEL && type != WATER ) )
            {
                water_surface_mesh_vertices.Resize( water_surface_mesh_vertices.Size() + 6 );
                v= water_surface_mesh_vertices.Data() + water_surface_mesh_vertices.Size() - 6;
                v[0].coord[0]= 3 * ( b->x + X );
                v[1].coord[0]= v[5].coord[0]= v[0].coord[0] + 1;
                v[2].coord[0]= v[4].coord[0]= v[0].coord[0] + 3;
                v[3].coord[0]= v[0].coord[0] + 4;

                v[0].coord[1]= v[3].coord[1]= 2 * ( b->y + Y ) - ((b->x)&1) + 2;
                v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;
                v[4].coord[1]= v[5].coord[1]= v[0].coord[1] - 1;

                h= ( (b->z -1) << 7 )+ ( b->LiquidLevel() * 128 / H_MAX_WATER_LEVEL );
                v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[4].coord[2]= v[5].coord[2]= h;

                unsigned char light= chunk->SunLightLevel( b->x, b->y, b->z + 1 );
                v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= v[4].light[0]= v[5].light[0]= light << 4;

                v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= v[4].light[1]= v[5].light[1]=
                chunk->FireLightLevel( b->x, b->y, b->z + 1 ) << 4;
            }// if water surface
        }//for
    }
}

/*
 __	   __
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
    short x, y, z;

    unsigned char t, t_up, t_fr, t_br, t_f;

    unsigned int quad_count= 0;

    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
        for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
        {
            t_up= chunk->Transparency( x, y, 1 );
            for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
            {
                t= t_up;
                t_fr= chunk->Transparency( x + 1, y + ( 1&(x+1) ), z );//forward right
                t_br= chunk->Transparency( x + 1, y - ( 1&x ), z );	//back right
                t_up= chunk->Transparency( x, y, z + 1 );//up
                t_f= chunk->Transparency( x, y + 1, z );//forward

#define ADD_QUADS \
                if( t != t_up ) \
                    quad_count+=2; \
                if( t != t_fr ) \
                    quad_count++; \
                if( t != t_br ) \
                    quad_count++; \
                if( t!= t_f ) \
                    quad_count++;
                ADD_QUADS
            }
        }

    //back chunk border( x E [ 0; H_CHUNK_WIDTH - 2 ], y=0 )
    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency(  x, 0, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( ! (x&1) )
                t_br= chunk->Transparency( x + 1, 0, z );//forward right
            else if( chunk_back != NULL )
                t_br= chunk_back->Transparency( x+ 1, H_CHUNK_WIDTH - 1, z );
            else
                t_br= t;
            t_fr= chunk->Transparency( x + 1, ( 1&(x+1) ), z );//forward right
            t_up= chunk->Transparency( x, 0, z + 1 );//up
            t_f= chunk->Transparency( x, 1, z );//forward
            ADD_QUADS
        }
    }

    //right chunk border ( y E [ 1; H_CHUNK_WIDTH - 2 ] )
    for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
    {
        t_up= chunk->Transparency(  H_CHUNK_WIDTH - 1, y, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( chunk_right != NULL )
            {
                t_fr= chunk_right->Transparency( 0, y, z );//forward right
                t_br= chunk_right->Transparency( 0, y - 1, z );	//back right
            }
            else
                t_fr= t_br= t;
            t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, y, z + 1 );//up
            t_f= chunk->Transparency( H_CHUNK_WIDTH - 1, y + 1, z );//forward
            ADD_QUADS
        }
    }

    //front chunk border ( x E[ 0; H_CHUNK_WIDTH - 2 ] )
    for( x= 0; x < H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency( x, H_CHUNK_WIDTH - 1, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( x&1  )
                t_fr= chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1, z );//forward right
            else if( chunk_front != NULL )
                t_fr= chunk_front->Transparency( x + 1, 0, z );//forward right
            else t_fr= t;

            t_br= chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1 - ( 1&x ), z );//back right
            t_up= chunk->Transparency( x, H_CHUNK_WIDTH - 1, z + 1 );//up

            if( chunk_front != NULL )
                t_f= chunk_front->Transparency( x, 0, z );//forward
            else
                t_f= t;

            ADD_QUADS
        }
    }

    //x= y= H_CHUNK_WIDTH - 1
    t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, H_CHUNK_WIDTH - 1, 1 );
    for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
    {
        t= t_up;
        if( chunk_front != NULL )
            t_f= chunk_front->Transparency( H_CHUNK_WIDTH - 1, 0, z );
        else
            t_f= t;

        if( chunk_right != NULL )
        {
            t_fr= chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 1, z );
            t_br= chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 2, z );
        }
        else
            t_fr= t_br= t;
        t_up= chunk->Transparency( H_CHUNK_WIDTH  - 1, H_CHUNK_WIDTH  - 1, z + 1 );//up
        ADD_QUADS

    }

    //x= H_CHUNK_WIDTH - 1, y=0
    t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, 0, 1 );
    for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
    {
        t= t_up;
        t_f= chunk->Transparency( H_CHUNK_WIDTH - 1, 1, z );

        if( chunk_right != NULL )
            t_fr= chunk_right->Transparency( 0, 0, z );
        else
            t_fr= t;

        if( chunk_back_right !=NULL )
            t_br= chunk_back_right->Transparency( 0, H_CHUNK_WIDTH - 1, z );
        else
            t_br= t;
        t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, 0, z + 1 );//up
        ADD_QUADS
    }

    chunk_vb.new_vertex_count= quad_count * 4;

#undef ADD_QUADS
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
                       1+--+2 - z-1
                        |  |
                        |  |
                       0+__+3 - z
                       /    \
                      /______\
                      \      /
                       \____/
forward side*/

/* old code of tex_coord generation for upper prism side

v[0].tex_coord[0]= v[0].coord[0] & 127;\
                    v[1].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 1;\
                    v[2].tex_coord[0]= v[6].tex_coord[0]= v[0].tex_coord[0] + 3;\
                    v[3].tex_coord[0]= v[0].tex_coord[0] + 4;\
\
                    v[0].tex_coord[1]= v[3].tex_coord[1]= ( v[0].coord[1] & 127 ) + 1;\
                    v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + 1;\
                    v[6].tex_coord[1]= v[7].tex_coord[1]= v[0].tex_coord[1] - 1;\
*/
/* new code
					v[0].tex_coord[0]= 0;\
                    v[1].tex_coord[0]= v[7].tex_coord[0]= 1;\
                    v[2].tex_coord[0]= v[6].tex_coord[0]= 3;\
                    v[3].tex_coord[0]= 4;\
\
                    v[0].tex_coord[1]= v[3].tex_coord[1]= 1;\
                    v[1].tex_coord[1]= v[2].tex_coord[1]= 2;\
                    v[6].tex_coord[1]= v[7].tex_coord[1]= 0;\

*/

void r_ChunkInfo::BuildChunkMesh()
{
    short x, y, z;

    unsigned char t, t_up, t_fr, t_br, t_f;//block transparency
    unsigned char normal_id;
    unsigned char v13[2];
    unsigned char tex_id, tex_scale, light[2];

    r_WorldVertex* v= chunk_vb.vb_data;
    r_WorldVertex* tmp_v;
    r_WorldVertex tmp_vertex;

    h_Block* b;

    short X= chunk->Longitude() * H_CHUNK_WIDTH, Y= chunk->Latitude() * H_CHUNK_WIDTH;
    short relative_X, relative_Y;
    h_World* w= chunk->GetWorld();
    relative_X= ( chunk->Longitude() - w->Longitude() ) * H_CHUNK_WIDTH;
    relative_Y= ( chunk->Latitude() - w->Latitude() ) * H_CHUNK_WIDTH;

    chunk_vb.water_vb_data= chunk_vb.vb_data + chunk_vb.new_vertex_count - 8;
    chunk_vb.water_vertex_count= 0;

    bool flat_lighting= chunk->IsEdgeChunk();

    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
    {
        for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
        {
            t_up= chunk->Transparency( x, y, 1 );
            for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
            {
                t= t_up;
                t_fr= chunk->Transparency( x + 1, y + ( 1&(x+1) ), z );//forward right
                t_br= chunk->Transparency( x + 1, y - ( 1&x ), z );	//back right
                t_up= chunk->Transparency( x, y, z + 1 );//up
                t_f= chunk->Transparency( x, y + 1, z );//forward

                if( t != t_up )//up
                {
#define BUILD_QUADS_UP \
                    if( t > t_up )\
                    {\
                        normal_id= DOWN;\
                        b= chunk->GetBlock( x, y, z  + 1 );\
                        light[0]= chunk->SunLightLevel( x, y, z );\
                        light[1]= chunk->FireLightLevel( x, y, z );\
                    }\
                    else\
                    {\
                        normal_id= UP;\
                        b= chunk->GetBlock( x, y, z  );\
                        light[0]= chunk->SunLightLevel( x, y, z + 1 );\
                        light[1]= chunk->FireLightLevel( x, y, z + 1 );\
                    }\
\
					\
					tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );\
					tex_scale= r_TextureManager::GetTextureScale( tex_id );\
					\
                    v[0].coord[0]= 3 * ( x + X );\
                    v[1].coord[0]= v[4].coord[0]= v[0].coord[0] + 1;\
                    v[2].coord[0]= v[7].coord[0]= v[0].coord[0] + 3;\
                    v[3].coord[0]= v[0].coord[0] + 4;\
\
                    v[0].coord[1]= v[3].coord[1]= 2 * ( y + Y ) - (x&1) + 2;\
                    v[1].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;\
                    v[7].coord[1]= v[4].coord[1]= v[0].coord[1] - 1;\
\
                    v[0].coord[2]= v[1].coord[2]= v[2].coord[2]= v[3].coord[2]= v[7].coord[2]= v[4].coord[2]= z;\
\
					if( r_TextureManager::TexturePerBlock( tex_id ) )\
					{\
						static const short hex_tex_coord[]=\
						{ 0, H_MAX_TEXTURE_SCALE,  1, H_MAX_TEXTURE_SCALE*2,   3, H_MAX_TEXTURE_SCALE*2,\
						4, H_MAX_TEXTURE_SCALE,   3, 0,   1,0 };\
						unsigned int hex_rotation= 0;\
                    	v[0].tex_coord[0]= 0;\
                    	v[1].tex_coord[0]= v[4].tex_coord[0]= 1*H_MAX_TEXTURE_SCALE;\
                    	v[2].tex_coord[0]= v[7].tex_coord[0]= 3*H_MAX_TEXTURE_SCALE;\
                    	v[3].tex_coord[0]= 4*H_MAX_TEXTURE_SCALE;\
\
                    	v[0].tex_coord[1]= v[3].tex_coord[1]= 1*H_MAX_TEXTURE_SCALE;\
                    	v[1].tex_coord[1]= v[2].tex_coord[1]= 2*H_MAX_TEXTURE_SCALE;\
                    	v[7].tex_coord[1]= v[4].tex_coord[1]= 0;\
					}\
					else\
					{\
						v[0].tex_coord[0]= tex_scale * v[0].coord[0];\
                    	v[1].tex_coord[0]= v[4].tex_coord[0]= v[0].tex_coord[0] + 1*tex_scale;\
                    	v[2].tex_coord[0]= v[7].tex_coord[0]= v[0].tex_coord[0] + 3*tex_scale;\
                    	v[3].tex_coord[0]= v[0].tex_coord[0] + 4*tex_scale;\
\
                    	v[0].tex_coord[1]= v[3].tex_coord[1]= tex_scale * v[0].coord[1];\
                    	v[1].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] + 1*tex_scale;\
                    	v[7].tex_coord[1]= v[4].tex_coord[1]= v[0].tex_coord[1] - 1*tex_scale;\
					}\
\
                    v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= v[7].normal_id= v[4].normal_id= normal_id;\
                    v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= v[7].tex_coord[2]= v[4].tex_coord[2]= \
                    tex_id;\
                    if( flat_lighting )\
                    	v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= v[7].light[0]= v[4].light[0]= light[0] << 4;\
					else\
					{\
                    	v[0].light[0]= w->GetForwardVertexSunLight( x + relative_X - 1, y + relative_Y - (x&1), z );\
                    	v[1].light[0]= w->GetBackVertexSunLight( x + relative_X, y + relative_Y + 1, z );\
                    	v[2].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y, z );\
                    	v[3].light[0]= w->GetBackVertexSunLight( x + relative_X + 1, y + relative_Y + ((1+x)&1), z );\
                    	v[7].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y - 1, z );\
                    	v[4].light[0]= w->GetBackVertexSunLight(  x + relative_X, y + relative_Y, z );\
					}\
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= v[7].light[1]= v[4].light[1]= light[1] << 4;\
                    v[5]= v[0];\
                    v[6]= v[3];\
\
                    if( normal_id == DOWN )\
                    {\
                        tmp_vertex= v[1];\
                        v[1]= v[3];\
                        v[3]= tmp_vertex;\
\
                        tmp_vertex= v[5];\
                        v[5]= v[7];\
                        v[7]= tmp_vertex;\
                    }\
                    v+=8;\


                    BUILD_QUADS_UP
                }

                if( t != t_fr )//forwaed right
                {
#define BUILD_QUADS_FORWARD_RIGHT \
\
					tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );\
					tex_scale= r_TextureManager::GetTextureScale( tex_id );\
\
                    v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( x + X ) + 3;\
                    v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;\
\
                    v[0].coord[1]= v[ 3 ].coord[1]= 2 * ( y + Y ) - (x&1) + 2;\
                    v[ 1 ].coord[1]= v[2].coord[1]= v[0].coord[1] + 1;\
\
                    v[0].coord[2]= v[ 1 ].coord[2]= z;\
                    v[2].coord[2]= v[ 3 ].coord[2]= z - 1;\
\
\
                    v[ 1 ].tex_coord[0]= v[2].tex_coord[0]= tex_scale * ( v[ 1 ].coord[1] - v[1].coord[0] );\
                    v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[ 1 ].tex_coord[0] - 2 * tex_scale;\
\
                    v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * 2 * tex_scale;\
                    v[2].tex_coord[1]= v[ 3 ].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;\
\
                    v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=\
                    tex_id;\
					if( flat_lighting )\
						v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;\
					else\
					{\
						v[0].light[0]= w->GetBackVertexSunLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z );\
						v[ 1 ].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y, z );\
						v[2].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y, z-1 );\
						v[ 3 ].light[0]= w->GetBackVertexSunLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z-1 );\
					}\
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;\
					v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;\
					if( normal_id == BACK_LEFT )\
					{\
						tmp_vertex= v[3];\
						v[3]= v[1];\
						v[1]= tmp_vertex;\
					}\
\
                    v+=4;
                    if( t > t_fr )
                    {
                        normal_id= BACK_LEFT;
                        b= chunk->GetBlock( x + 1, y + ((x+1)&1), z );
                        light[0]= chunk->SunLightLevel( x, y, z );
                        light[1]= chunk->FireLightLevel( x, y, z );
                    }
                    else
                    {
                        normal_id= FORWARD_RIGHT;
                        b= chunk->GetBlock( x, y, z );
                        light[0]= chunk->SunLightLevel( x + 1, y + ((x+1)&1), z );
                        light[1]= chunk->FireLightLevel( x + 1, y + ((x+1)&1), z );
                    }
                    BUILD_QUADS_FORWARD_RIGHT
                }

                if( t != t_br )//back right
                {
#define BUILD_QUADS_BACK_RIGHT \
\
					tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );\
					tex_scale= r_TextureManager::GetTextureScale( tex_id );\
\
                    v[ 1 ].coord[0]= v[2].coord[0]= 3 * ( x + X ) + 3;\
                    v[0].coord[0]= v[ 3 ].coord[0]= v[ 1 ].coord[0] + 1;\
\
                    v[ 1 ].coord[1]= v[2].coord[1]= 2 * ( y + Y ) - (x&1) + 2 - 1;\
                    v[0].coord[1]= v[ 3 ].coord[1]= v[ 1 ].coord[1] + 1;\
\
                    v[ 1 ].coord[2]= v[0].coord[2]= z;\
                    v[2].coord[2]= v[ 3 ].coord[2]= z - 1;\
\
                   \
                    v[2].tex_coord[0]= v[ 1 ].tex_coord[0]=  ( v[1].coord[1]  + v[1].coord[0] ) * tex_scale;\
                    v[0].tex_coord[0]= v[ 3 ].tex_coord[0]= v[2].tex_coord[0] + 2 * tex_scale;\
\
                    v[0].tex_coord[1]= v[ 1 ].tex_coord[1]= z * 2 * tex_scale;\
                    v[ 3 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;\
\
                    v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=\
                    tex_id;\
					if( flat_lighting )\
                    	v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;\
					else\
					{\
						v[0].light[0]= w->GetBackVertexSunLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z );\
						v[3].light[0]= w->GetBackVertexSunLight( x + relative_X + 1, y + relative_Y + ((x+1)&1), z - 1 );\
						v[2].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y - 1, z - 1 );\
						v[1].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y - 1, z );\
					}\
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;\
                    v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;\
                    if( normal_id == BACK_RIGHT )\
                    {\
                    	tmp_vertex= v[3];\
						v[3]= v[1];\
						v[1]= tmp_vertex;\
                    }\
                    v+=4;
                    if( t > t_br )
                    {
                        normal_id= FORWARD_LEFT;
                        b= chunk->GetBlock( x + 1, y - (x&1), z );
                        light[0]= chunk->SunLightLevel( x, y, z );
                        light[1]= chunk->FireLightLevel( x, y, z );
                    }
                    else
                    {
                        normal_id= BACK_RIGHT;
                        b= chunk->GetBlock( x, y, z );
                        light[0]= chunk->SunLightLevel( x + 1, y - (x&1), z );
                        light[1]= chunk->FireLightLevel( x + 1, y - (x&1), z );
                    }
                    BUILD_QUADS_BACK_RIGHT
                }

                if( t != t_f )//forward
                {
#define BUILD_QUADS_FORWARD\
\
					tex_id= r_TextureManager::GetTextureId( b->Type(), normal_id );\
					tex_scale= r_TextureManager::GetTextureScale( tex_id );\
\
                    v[0].coord[0]= v[ 1 ].coord[0]= 3 * ( x + X ) + 1;\
                    v[0].coord[1]= v[ 1 ].coord[1]= v[2].coord[1]= v[ 3 ].coord[1]= 2 * ( y + Y ) - (x&1) + 2 + 1;\
\
                    v[0].coord[2]= v[ 3 ].coord[2]= z;\
                    v[ 1 ].coord[2]= v[2].coord[2]= z - 1;\
\
                    v[ 3 ].coord[0]= v[2].coord[0]= v[ 1 ].coord[0] + 2;\
\
\
                    v[0].tex_coord[0]= v[ 1 ].tex_coord[0]= v[0].coord[0] * tex_scale;\
                    v[2].tex_coord[0]= v[ 3 ].tex_coord[0]= v[0].tex_coord[0] + 2 * tex_scale;\
\
                    v[0].tex_coord[1]= v[ 3 ].tex_coord[1]= z * 2 * tex_scale;\
                    v[ 1 ].tex_coord[1]= v[2].tex_coord[1]= v[0].tex_coord[1] - 2 * tex_scale;\
                    v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]=\
                    tex_id;\
                    \
                    if( flat_lighting )\
                    	v[0].light[0]= v[1].light[0]= v[2].light[0]= v[3].light[0]= light[0] << 4;\
					else\
					{\
						v[0].light[0]= w->GetBackVertexSunLight( x + relative_X, y + relative_Y + 1, z );\
						v[1].light[0]= w->GetBackVertexSunLight( x + relative_X, y + relative_Y + 1, z - 1 );\
						v[2].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y, z - 1 );\
						v[3].light[0]= w->GetForwardVertexSunLight( x + relative_X, y + relative_Y, z );\
					}\
					v[0].light[1]= v[1].light[1]= v[2].light[1]= v[3].light[1]= light[1] << 4;\
                    v[0].normal_id= v[1].normal_id= v[2].normal_id= v[3].normal_id= normal_id;\
					if( normal_id == BACK )\
                    {\
                    	tmp_vertex= v[3];\
						v[3]= v[1];\
						v[1]= tmp_vertex;\
                    }\
                    v+= 4;
                    if( t > t_f )
                    {
                        normal_id= BACK;
                        b= chunk->GetBlock( x, y + 1, z );
                        light[0]= chunk->SunLightLevel( x, y, z );
                        light[1]= chunk->FireLightLevel( x, y, z );
                    }
                    else
                    {
                        normal_id= FORWARD;
                        b= chunk->GetBlock( x, y, z );
                        light[0]= chunk->SunLightLevel( x, y+1, z );
                        light[1]= chunk->FireLightLevel( x, y+1, z );
                    }
                    BUILD_QUADS_FORWARD
                }
            }
        }
    }

#if 1
    //back chunk border( x E [ 0; H_CHUNK_WIDTH - 2 ], y=0 )
    y= 0;
    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency(  x, 0, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( ! (x&1) )
                t_br= chunk->Transparency( x + 1, 0, z );//forward right
            else if( chunk_back != NULL )
                t_br= chunk_back->Transparency( x + 1, H_CHUNK_WIDTH - 1, z );
            else
                t_br= t;
            t_fr= chunk->Transparency( x + 1, ( 1&(x+1) ), z );//forward right
            t_up= chunk->Transparency( x, 0, z + 1 );//up
            t_f= chunk->Transparency( x, 1, z );//forward

            if( t!= t_up )
            {
                BUILD_QUADS_UP
            }
            if( t!= t_fr )
            {
                if( t > t_fr )
                {
                    normal_id= BACK_LEFT;
                    b= chunk->GetBlock( x + 1, y + ((x+1)&1), z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk->SunLightLevel(  x + 1, y + ((x+1)&1), z );
                    light[1]= chunk->FireLightLevel(  x + 1, y + ((x+1)&1), z );
                }
                BUILD_QUADS_FORWARD_RIGHT
            }
            if( t!= t_br )
            {
                if( t > t_br )
                {
                    normal_id= FORWARD_LEFT;
                    b= (x&1) ? chunk_back->GetBlock( x + 1, H_CHUNK_WIDTH - 1, z ) :
                       chunk->GetBlock( x + 1, 0, z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= BACK_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= (x&1) ? chunk_back->SunLightLevel( x + 1, H_CHUNK_WIDTH - 1, z ) :
                           chunk->SunLightLevel( x + 1, 0, z );

					light[1]= (x&1) ? chunk_back->FireLightLevel( x + 1, H_CHUNK_WIDTH - 1, z ) :
                           chunk->FireLightLevel( x + 1, 0, z );
                }
                BUILD_QUADS_BACK_RIGHT
            }
            if( t!= t_f )
            {
                if( t > t_f )
                {
                    normal_id= BACK;
                    b= chunk->GetBlock( x, y + 1, z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk->SunLightLevel( x, y + 1, z );
                    light[1]= chunk->FireLightLevel( x, y + 1, z );
                }
                BUILD_QUADS_FORWARD
            }

        }
    }
#endif
#if 1
    //right chunk border ( y E [ 1; H_CHUNK_WIDTH - 2 ] )
    x= H_CHUNK_WIDTH - 1;
    for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
    {
        t_up= chunk->Transparency(  H_CHUNK_WIDTH - 1, y, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( chunk_right != NULL )
            {
                t_fr= chunk_right->Transparency( 0, y, z );//forward right
                t_br= chunk_right->Transparency( 0, y - 1, z );	//back right
            }
            else
                t_fr= t_br= t;
            t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, y, z + 1 );//up
            t_f= chunk->Transparency( H_CHUNK_WIDTH - 1, y + 1, z );//forward

            if( t!= t_up )
            {
                BUILD_QUADS_UP
            }
            if( t!= t_fr )
            {
                if( t > t_fr )
                {
                    normal_id= BACK_LEFT;
                    b= chunk_right->GetBlock( 0, y + ((x+1)&1), z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk_right->SunLightLevel( 0, y + ((x+1)&1), z );
                    light[1]= chunk_right->FireLightLevel( 0, y + ((x+1)&1), z );
                }
                BUILD_QUADS_FORWARD_RIGHT
            }
            if( t!= t_br )
            {
                if( t > t_br )
                {
                    normal_id= FORWARD_LEFT;
                    b= chunk_right->GetBlock( 0, y - (x&1), z ) ;
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= BACK_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk_right->SunLightLevel( 0, y - (x&1), z ) ;
                    light[1]= chunk_right->FireLightLevel( 0, y - (x&1), z ) ;
                }
                BUILD_QUADS_BACK_RIGHT
            }
            if( t!= t_f )
            {
                if( t > t_f )
                {
                    normal_id= BACK;
                    b= chunk->GetBlock( x, y + 1, z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk->SunLightLevel( x, y + 1, z );
                    light[1]= chunk->FireLightLevel( x, y + 1, z );
                }
                BUILD_QUADS_FORWARD
            }
        }
    }
#endif
#if 1
    //front chunk border ( x E[ 0; H_CHUNK_WIDTH - 2 ] )
    y= H_CHUNK_WIDTH - 1;
    for( x= 0; x < H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency( x, H_CHUNK_WIDTH - 1, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
            t= t_up;
            if( x&1  )
                t_fr= chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1, z );//forward right
            else if( chunk_front != NULL )
                t_fr= chunk_front->Transparency( x + 1, 0, z );//forward right
            else t_fr= t;

            t_br= chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1 - ( 1&x ), z );//back right
            t_up= chunk->Transparency( x, H_CHUNK_WIDTH - 1, z + 1 );//up

            if( chunk_front != NULL )
                t_f= chunk_front->Transparency( x, 0, z );//forward
            else
                t_f= t;

            if( t!= t_up )
            {
                BUILD_QUADS_UP
            }
            if( t!= t_fr )
            {
                if( t > t_fr )
                {
                    normal_id= BACK_LEFT;
                    b= ( x&1) ? chunk->GetBlock( x + 1, H_CHUNK_WIDTH - 1, z ) :
                       chunk_front->GetBlock( x + 1, 0, z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= ( x&1) ? chunk->SunLightLevel( x + 1, H_CHUNK_WIDTH - 1, z ) :
                           chunk_front->SunLightLevel( x + 1, 0, z );
					light[1]= ( x&1) ? chunk->FireLightLevel( x + 1, H_CHUNK_WIDTH - 1, z ) :
                           chunk_front->FireLightLevel( x + 1, 0, z );
                }
                BUILD_QUADS_FORWARD_RIGHT
            }
            if( t!= t_br )
            {
                if( t > t_br )
                {
                    normal_id= FORWARD_LEFT;
                    b= chunk->GetBlock( x + 1, y - (x&1), z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= BACK_RIGHT;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk->SunLightLevel( x + 1, y - (x&1), z );
                    light[1]= chunk->FireLightLevel( x + 1, y - (x&1), z );
                }
                BUILD_QUADS_BACK_RIGHT
            }
            if( t!= t_f )
            {
                if( t > t_f )
                {
                    normal_id= BACK;
                    b= chunk_front->GetBlock( x, 0, z );
                    light[0]= chunk->SunLightLevel( x, y, z );
                    light[1]= chunk->FireLightLevel( x, y, z );
                }
                else
                {
                    normal_id= FORWARD;
                    b= chunk->GetBlock( x, y, z );
                    light[0]= chunk_front->SunLightLevel( x, 0, z );
                    light[1]= chunk_front->FireLightLevel( x, 0, z );
                }
                BUILD_QUADS_FORWARD
            }
        }
    }
#endif
#if 1
    //right up chunk corner
    x= y= H_CHUNK_WIDTH - 1;
    t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, H_CHUNK_WIDTH - 1, 1 );
    for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
    {
        t= t_up;
        if( chunk_front != NULL )
            t_f= chunk_front->Transparency( H_CHUNK_WIDTH - 1, 0, z );
        else
            t_f= t;

        if( chunk_right != NULL )
        {
            t_fr= chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 1, z );
            t_br= chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 2, z );
        }
        else
            t_fr= t_br= t;
        t_up= chunk->Transparency( H_CHUNK_WIDTH  - 1, H_CHUNK_WIDTH  - 1, z + 1 );//up

        if( t!= t_up )
        {
            BUILD_QUADS_UP
        }
        if( t!= t_fr )
        {
            if( t > t_fr )
            {
                normal_id= BACK_LEFT;
                b= chunk_right->GetBlock( 0, H_CHUNK_WIDTH  - 1, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= FORWARD_RIGHT;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk_right->SunLightLevel( 0, H_CHUNK_WIDTH  - 1, z );
                light[1]= chunk_right->FireLightLevel( 0, H_CHUNK_WIDTH  - 1, z );
            }
            BUILD_QUADS_FORWARD_RIGHT
        }
        if( t!= t_br )
        {
            if( t > t_br )
            {
                normal_id= FORWARD_LEFT;
                b= chunk_right->GetBlock( 0, H_CHUNK_WIDTH  - 2, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= BACK_RIGHT;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk_right->SunLightLevel( 0, H_CHUNK_WIDTH  - 2, z );
                light[1]= chunk_right->FireLightLevel( 0, H_CHUNK_WIDTH  - 2, z );
            }
            BUILD_QUADS_BACK_RIGHT
        }
        if( t!= t_f )
        {
            if( t > t_f )
            {
                normal_id= BACK;
                b= chunk_front->GetBlock( x, 0, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= FORWARD;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk_front->SunLightLevel( x, 0, z );
                light[1]= chunk_front->FireLightLevel( x, 0, z );
            }
            BUILD_QUADS_FORWARD
        }
    }
#endif
#if 1
    //right down chunk corner
    x= H_CHUNK_WIDTH - 1, y=0;
    t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, 0, 1 );
    for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
    {
        t= t_up;
        t_f= chunk->Transparency( H_CHUNK_WIDTH - 1, 1, z );

        if( chunk_right != NULL )
            t_fr= chunk_right->Transparency( 0, 0, z );
        else
            t_fr= t;

        if( chunk_back_right !=NULL )
            t_br= chunk_back_right->Transparency( 0, H_CHUNK_WIDTH - 1, z );
        else
            t_br= t;
        t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, 0, z + 1 );//up

        if( t!= t_up )
        {
            BUILD_QUADS_UP
        }
        if( t!= t_fr )
        {
            if( t > t_fr )
            {
                normal_id= BACK_LEFT;
                v13[0]= 3;
                v13[1]= 1;
                b= chunk_right->GetBlock( 0, 0, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= FORWARD_RIGHT;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk_right->SunLightLevel( 0, 0, z );
                light[1]= chunk_right->FireLightLevel( 0, 0, z );
            }
            BUILD_QUADS_FORWARD_RIGHT
        }
        if( t!= t_br )
        {
            if( t > t_br )
            {
                normal_id= FORWARD_LEFT;
                b= chunk_back_right->GetBlock( 0, H_CHUNK_WIDTH  - 1, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= BACK_RIGHT;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk_back_right->SunLightLevel( 0, H_CHUNK_WIDTH  - 1, z );
                light[1]= chunk_back_right->FireLightLevel( 0, H_CHUNK_WIDTH  - 1, z );
            }
            BUILD_QUADS_BACK_RIGHT
        }
        if( t!= t_f )
        {
            if( t > t_f )
            {
                normal_id= BACK;
                b= chunk->GetBlock( x, y + 1, z );
                light[0]= chunk->SunLightLevel( x, y, z );
                light[1]= chunk->FireLightLevel( x, y, z );
            }
            else
            {
                normal_id= FORWARD;
                v13[0]= 1;
                b= chunk->GetBlock( x, y, z );
                light[0]= chunk->SunLightLevel( x, y + 1, z );
                light[1]= chunk->FireLightLevel( x, y + 1, z );
            }
            BUILD_QUADS_FORWARD
        }
    }
#endif


    chunk_vb.water_vb_data+= 8;
}

#endif//CHUNK_INFO_CPP
