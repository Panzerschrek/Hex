#include "chunk_phys_mesh.hpp"


void h_ChunkPhysMesh::BuildMesh( h_Chunk* chunk, h_Chunk* chunk_front, h_Chunk *chunk_right,
                                 h_Chunk *chunk_back_right,h_Chunk *chunk_back, short z_min, short z_max )
{
    this->z_min= z_min;
    this->z_max= z_max;

    block_sides.Resize(0);
    upper_block_faces.Resize(0);

    short x, y, z;
    short X, Y;
    X= chunk->Longitude() * H_CHUNK_WIDTH;
    Y= chunk->Latitude() * H_CHUNK_WIDTH;
    unsigned char t, t_up, t_fr, t_br, t_f;

    p_UpperBlockFace* block_face;
    p_BlockSide* block_side;


    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
        for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
        {
            t_up= chunk->Transparency( x, y, z_min );
            for( z= z_min; z<= z_max; z++ )
            {

#define BUILD_SUBMESH( transparency_func_fr, transparency_func_br, transparency_func_f )\
                t= t_up;\
                t_fr= transparency_func_fr;\
                t_br= transparency_func_br;\
                t_up= chunk->Transparency( x, y, z + 1 );\
                t_f= transparency_func_f;\
\
                if( t != t_up )\
                {\
                    upper_block_faces.Resize( upper_block_faces.Size()+1 );\
                    block_face= &upper_block_faces.Data()[ upper_block_faces.Size() - 1 ];\
                    if( t > t_up )\
                        block_face->Gen( x + X, y + Y, z + 1, DOWN );\
                    else\
                        block_face->Gen( x + X, y + Y, z, UP );\
                }\
                if( t != t_fr )\
                {\
                    block_sides.Resize( block_sides.Size()+1 );\
                    block_side= &block_sides.Data()[ block_sides.Size() - 1 ];\
                    if( t > t_fr )\
                        block_side->Gen( x + X + 1, y + Y + ((x+1)&1), z-1, BACK_LEFT );\
                    else\
                        block_side->Gen( x + X, y + Y, z-1, FORWARD_RIGHT );\
                }\
                if( t != t_br )\
                {\
                    block_sides.Resize( block_sides.Size()+1 );\
                    block_side= &block_sides.Data()[ block_sides.Size() - 1 ];\
                    if( t > t_br )\
                        block_side->Gen( x + X + 1, y + Y - (x&1), z-1, FORWARD_LEFT );\
                    else\
                        block_side->Gen( x + X, y + Y, z-1, BACK_RIGHT );\
                }\
\
                if( t!= t_f )\
                {\
                    block_sides.Resize( block_sides.Size() + 1 );\
                    block_side= &block_sides.Data()[ block_sides.Size() - 1 ];\
                    if( t > t_f )\
                        block_side->Gen( x + X, y + Y + 1, z-1, BACK );\
                    else\
                        block_side->Gen( x + X, y + Y, z-1, FORWARD );\
                }\

                BUILD_SUBMESH( chunk->Transparency( x + 1, y + ( 1&(x+1) ), z ),
                               chunk->Transparency( x + 1, y - ( 1&x ), z ),
                               chunk->Transparency( x, y + 1, z ) );

            }// for z
        }//for y


    //right chunk border ( y E [ 1; H_CHUNK_WIDTH - 2 ] )
    x= H_CHUNK_WIDTH - 1;
    for( y= 1; y< H_CHUNK_WIDTH - 1; y++ )
    {
        t_up= chunk->Transparency( H_CHUNK_WIDTH - 1, y, z_min );
        for( z= z_min; z<= z_max; z++ )
        {
            BUILD_SUBMESH( chunk_right->Transparency( 0, y, z ),
                           chunk_right->Transparency( 0, y - 1, z ),
                           chunk->Transparency( H_CHUNK_WIDTH - 1, y + 1, z ) );

        }
    }

    //front chunk border ( x E[ 0; H_CHUNK_WIDTH - 2 ] )
    y= H_CHUNK_WIDTH - 1;
    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency( x, H_CHUNK_WIDTH - 1, z_min );
        for( z= z_min; z<= z_max; z++ )
        {
            BUILD_SUBMESH( (x&1) ? chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1, z ) : chunk_front->Transparency( x + 1, 0, z ),
                           chunk->Transparency( x + 1, H_CHUNK_WIDTH - 1 - ( 1&x ), z ),
                           chunk_front->Transparency( x, 0, z ) );
        }
    }

    //back chunk border( x E [ 0; H_CHUNK_WIDTH - 2 ], y=0 )
    y= 0;
    for( x= 0; x< H_CHUNK_WIDTH - 1; x++ )
    {
        t_up= chunk->Transparency(  x, 0, 1 );
        for( z= 0; z< H_CHUNK_HEIGHT - 2; z++ )
        {
        	BUILD_SUBMESH( chunk->Transparency( x + 1, ( 1&(x+1) ), z ),
						(x&1) ? chunk_back->Transparency( x + 1, H_CHUNK_WIDTH - 1, z ) : chunk->Transparency( x + 1, 0, z ),
						chunk->Transparency( x, 1, z ) );

        }
    }


    //right up chunk corner
    x= y= H_CHUNK_WIDTH - 1;
    t_up= chunk->Transparency( x, y, z_min );
    for( z= z_min; z<= z_max; z++ )
    {
        BUILD_SUBMESH( chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 1, z ),
                       chunk_right->Transparency( 0, H_CHUNK_WIDTH  - 2, z ),
                       chunk_front->Transparency( H_CHUNK_WIDTH - 1, 0, z  ) );
    }


    //right down chunk corner
    x= H_CHUNK_WIDTH - 1, y=0;
     t_up= chunk->Transparency( x, y, z_min );
    for( z= z_min; z<= z_max; z++ )
    {
        BUILD_SUBMESH( chunk_right->Transparency( 0, 0, z ),
                       chunk_back_right->Transparency( 0, H_CHUNK_WIDTH - 1, z ),
                       chunk->Transparency( H_CHUNK_WIDTH - 1, 1, z ) );
    }

}
