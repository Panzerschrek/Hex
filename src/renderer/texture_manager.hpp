#pragma once

#include "../fwd.hpp"
#include "../hex.hpp"
#include "panzer_ogl_lib.hpp"
#include "texture.hpp"

class r_TextureManager
{
public:
	r_TextureManager( unsigned int detalization, bool filter_textures );
	~r_TextureManager();

	static unsigned char GetTextureId( h_BlockType block_type, unsigned char normal_id );

	// Scale, divedid by 4.
	// 4 means 1 texture per block.
	// 1 means 1 texture for 4 blocks.
	// 16 means 4 textures per block.
	static unsigned char GetTextureScale( unsigned char tex_id );
	static bool TexturePerBlock( unsigned char tex_id );

	void BindTextureArray( unsigned int unit );
	void BindWaterTexture( unsigned int unit );

private:
	void InitTextureTable();
	void LoadWorldTextures();

private:
	static unsigned char texture_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];
	static bool texture_mode_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ]; // if( true ) - mode per block, else mode - projection
	static unsigned char texture_scale_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];

	// OpenGL id for array texture
	GLuint texture_array_;
	r_Texture water_texture_;

	const unsigned int texture_detalization_;
	const bool filter_textures_;
};

inline unsigned char r_TextureManager::GetTextureId( h_BlockType block_type, unsigned char normal_id )
{
	return texture_table_[ ( static_cast<size_t>(block_type) << 3 ) | normal_id ];
}

inline  bool r_TextureManager::TexturePerBlock( unsigned char tex_id )
{
	return texture_mode_table_[ tex_id ];
}

inline unsigned char r_TextureManager::GetTextureScale( unsigned char tex_id )
{
	return texture_scale_table_[ tex_id ];
}
