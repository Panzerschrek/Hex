#ifndef TEXTURE_MANAGER_CPP
#define TEXTURE_MANAGER_CPP
#include "texture_manager.hpp"
#include "../block.hpp"
#include <iostream>
#include <fstream>

unsigned char r_TextureManager::texture_table[ NUM_BLOCK_TYPES * 8 ];
bool r_TextureManager::texture_mode_table[ NUM_BLOCK_TYPES * 8 ];
unsigned char r_TextureManager::texture_scale_table[ NUM_BLOCK_TYPES * 8 ];

void r_TextureManager::InitTextureTable()
{
	unsigned int i;
    for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
        texture_table[i]= 0;
	for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
		texture_mode_table[i]= false;
	for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
		texture_scale_table[i]= H_MAX_TEXTURE_SCALE;
}

r_TextureManager::r_TextureManager()
{
    InitTextureTable();
}


void r_TextureManager::LoadTextures()
{
    int max_tex_size, atlas_size;
    float max_lod;

    unsigned int tex_id= 0;
    r_TextureFile tf;
    char str[256];

    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &max_tex_size );



#ifdef OGL21
    max_lod= 8.0f;
    atlas_size= 4096;//min( 256 * 16, max_tex_size );
    texture_atlas.Create();
    texture_atlas.TextureData( atlas_size, atlas_size, GL_UNSIGNED_BYTE, GL_RGBA, 32, NULL );
    texture_atlas.SetFiltration( GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST );
   	//texture_atlas.SetFiltration( GL_NEAREST, GL_NEAREST );
    texture_atlas.MoveOnGPU();
    texture_atlas.SetMaxLod( max_lod );


#else
    texture_array.TextureData( 256, 256, 32, GL_UNSIGNED_BYTE, GL_RGBA, 32, NULL );
    texture_array.MoveOnGPU();
#endif

    if( ! rLoadTextureTGA( &tf, "textures/null.tga" ) )
    {
#ifdef OGL21
        texture_atlas.TexSubData2GPU( (tex_id%16)*256, (tex_id/16)*256, 256, 256, tf.data );
        delete[] tf.data;
#else
        texture_array.TextureLayer( tex_id, tf.data );
        delete[] tf.data;
#endif
    }

    std::ifstream f( "textures/textures.cfg" );
    if( f.fail() )
    {
        printf( "error, file \"textures/textures.cfg\" not found\n" );
        return;
    }


    while( ! f.eof() )
    {
        f>>str;
        if( rLoadTextureTGA( &tf, str ) )
        {
            printf( "error, texture \"%s\" not fund\n", str );
        }
        else
        {
            tex_id++;
#ifdef OGL21
            texture_atlas.TexSubData2GPU( (tex_id%16)*256, (tex_id/16)*256, 256, 256, tf.data );
            delete[] tf.data;
#else
            texture_array.TextureLayer( tex_id, tf.data );
            delete[] tf.data;
#endif

        }

        f>>str;
        if( strcmp(str, "{" ) )
        {
            printf( "texture config parsing error\n" );
            return;
        }

        while( ! f.eof() )
        {
            f>>str;
            if( str[0] == '}' && str[1] == 0 )
                break;

            if( !strcmp( str, "blocks" ) )
            {
                f>>str;
                if( strcmp(str, "{" ) )
                {
                    printf( "texture config parsing error - blocks\n" );
                    return;
                }
                while( ! f.eof() )
                {
                    f>> str;
                    if( str[0] == '}' && str[1] == 0 )
                        break;

                    h_BlockType t= h_Block::GetGetBlockTypeByName( str );
                    // if( t!= BLOCK_UNKNOWN )
                    //   texture_table[ t<<3 ] = tex_id;

                    f>>str;
                    if( !strcmp ( str, "universal" ) )
                    {
                        for( int k= 0; k< 8; k++ )
                            texture_table[ (t<<3) | k ]= tex_id;
                    }

                    h_Direction d= h_Block::GetDirectionByName( str );
                    if( d != DIRECTION_UNKNOWN && t!= BLOCK_UNKNOWN  )
                        texture_table[ (t<<3) |d ]= tex_id;
                }
            }
            else if( ! strcmp( str, "per_block" ) )
            {
            	texture_mode_table[ tex_id ]= true;
            }
             else if( ! strcmp( str, "scale" ) )
            {
            	f>>str;
            	texture_scale_table[ tex_id ]= min( max( atoi(str), 1 ), H_MAX_TEXTURE_SCALE * H_MAX_TEXTURE_SCALE );
            }
            else
            {
                printf( "warning, unknown texture property: \"%s\"", str );
            }
        }

    }

    #ifndef OGL21
    texture_array.SetFiltration( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR );
    //texture_array.SetFiltration( GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST );
    texture_array.GenerateMipmap();
    #endif

}

#endif//TEXTURE_MANAGER_CPP
