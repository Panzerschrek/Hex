#pragma once
#include "../hex.hpp"
#include "ph.h"

class r_TextureManager
{
public:

	r_TextureManager();

	static unsigned char GetTextureId( h_BlockType block_type, unsigned char normal_id = FORWARD );
	static unsigned char GetTextureScale( unsigned char tex_id );

	static 	bool TexturePerBlock( unsigned char tex_id );
	void LoadTextures();
	void BindTextureArray( unsigned int unit= 0 );

	void SetTextureSize( int size );
	void SetFiltration( bool filter_textures );
private:
	void InitTextureTable();

	void DrawNullTexture( QImage* img, const char* text="" );

	static unsigned char texture_table[ NUM_BLOCK_TYPES * 8 ];
	static bool texture_mode_table [ NUM_BLOCK_TYPES * 8 ]; // if( true ) - mode per block, else mode - projection
	static unsigned char texture_scale_table[ NUM_BLOCK_TYPES * 8 ];

	// OpenGL id for array texture
	GLuint texture_array;

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
	//texture_array.Bind( unit );
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array );
}

inline void r_TextureManager::SetTextureSize( int size )
{
	texture_size= size;
}
inline void r_TextureManager::SetFiltration( bool filter_textures )
{
	this->filter_textures= filter_textures;
}
