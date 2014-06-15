#ifndef TEXTURE_MANAGER_CPP
#define TEXTURE_MANAGER_CPP
#include "texture_manager.hpp"
#include "rendering_constants.hpp"
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

	texture_size= R_MAX_TEXTURE_RESOLUTION/4;
    filter_textures= false;
}

void r_TextureManager::LoadTextures()
{
    unsigned int tex_id= 0;
    r_TextureFile tf[2];
    char str[256];

    tf[1].data= new unsigned char[ R_MAX_TEXTURE_RESOLUTION * R_MAX_TEXTURE_RESOLUTION * 4 ];

    texture_array.TextureData( texture_size, texture_size, 32, GL_UNSIGNED_BYTE, GL_RGBA, 32, NULL );
    texture_array.MoveOnGPU();

    if( ! rLoadTextureTGA( &tf[0], "textures/null.tga" ) )
    {
        int s= 0, d=1;
        for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
            rRGBAGetMip( &tf[s], &tf[d] );
        texture_array.TextureLayer( tex_id, tf[s].data );
        delete[] tf[0].data;
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
        if( rLoadTextureTGA( &tf[0], str ) )
            printf( "error, texture \"%s\" not fund\n", str );
        else
        {
            tex_id++;

            int s= 0, d=1;
            for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
                rRGBAGetMip( &tf[s], &tf[d] );
            texture_array.TextureLayer( tex_id, tf[s].data );
            delete[] tf[0].data;

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
                printf( "warning, unknown texture property: \"%s\"", str );
        }//while ! eof

    }//while ! eof

	delete[] tf[1].data;
	if( filter_textures )
    	texture_array.SetFiltration( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR );
	else
		texture_array.SetFiltration( GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST );
    texture_array.GenerateMipmap();
}

#endif//TEXTURE_MANAGER_CPP
