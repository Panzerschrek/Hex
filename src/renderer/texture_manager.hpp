#pragma once

#include "../hex.hpp"
#include "panzer_ogl_lib.hpp"

class r_TextureManager
{
public:
	r_TextureManager();

	static unsigned char GetTextureId( h_BlockType block_type, unsigned char normal_id );
	static unsigned char GetTextureScale( unsigned char tex_id );
	static bool TexturePerBlock( unsigned char tex_id );

	void LoadTextures();

	void BindTextureArray( unsigned int unit= 0 );
	void SetTextureSize( unsigned int size );
	void SetFiltration( bool filter_textures );

private:
	void InitTextureTable();

private:
	static unsigned char texture_table_[ NUM_BLOCK_TYPES * 8 ];
	static bool texture_mode_table_[ NUM_BLOCK_TYPES * 8 ]; // if( true ) - mode per block, else mode - projection
	static unsigned char texture_scale_table_[ NUM_BLOCK_TYPES * 8 ];

	// OpenGL id for array texture
	// TODO - clear OpenGL resources
	GLuint texture_array_;

	unsigned int texture_size_;
	bool filter_textures_;
};

inline unsigned char r_TextureManager::GetTextureId( h_BlockType block_type, unsigned char normal_id )
{
	return texture_table_[ ( block_type << 3 ) | normal_id ];
}

inline  bool r_TextureManager::TexturePerBlock( unsigned char tex_id )
{
	return texture_mode_table_[ tex_id ];
}

inline unsigned char r_TextureManager::GetTextureScale( unsigned char tex_id )
{
	return texture_scale_table_[ tex_id ];
}
