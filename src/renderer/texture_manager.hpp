#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP
#include "../hex.hpp"
#include "texture.h"
#include "texture_array.h"


class r_TextureManager
{
public:

    r_TextureManager();

    static unsigned char GetTextureId( h_BlockType block_type, unsigned char normal_id = FORWARD );
    static unsigned char GetTextureScale( unsigned char tex_id );

    static 	bool TexturePerBlock( unsigned char tex_id );
    void LoadTextures();
    void BindTextureArray( unsigned int unit= 0 );

private:
    void InitTextureTable();

    static unsigned char texture_table[ NUM_BLOCK_TYPES * 8 ];
    static bool texture_mode_table [ NUM_BLOCK_TYPES * 8 ]; // if( true ) - mode per block, else mode - projection
    static unsigned char texture_scale_table[ NUM_BLOCK_TYPES * 8 ];
/*#ifdef OGL21
    r_Texture texture_atlas;
#else*/
    r_TextureArray texture_array;
//#endif

	unsigned int texture_size;
	bool filter_textures;

};


inline unsigned char r_TextureManager::GetTextureId( h_BlockType block_type, unsigned char normal_id )
{
    return texture_table[ ( block_type<< 3 ) | normal_id ];
}


inline 	bool r_TextureManager::TexturePerBlock( unsigned char tex_id )
{
    return texture_mode_table[ tex_id ];
}

inline unsigned char r_TextureManager::GetTextureScale( unsigned char tex_id )
{
    return texture_scale_table[ tex_id ];
}
inline void r_TextureManager::BindTextureArray( unsigned int unit )
{
/*#ifdef OGL21
    texture_atlas.BindTexture( unit );
#else*/
    texture_array.Bind( unit );
//#endif
}
#endif//TEXTURE_MANAGER_HPP
