#pragma once
#include "../hex.hpp"

class r_Texture;

namespace r_ImgUtils
{
	void RGBA8_To_BRGA8( const unsigned char* in, unsigned char* out, int width, int height );
	void RGBA8_To_BRGA8( unsigned char* in_out, int width, int height );
	void R1_To_R8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char white_value= 255 );
	void R8_To_RGBA8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char alpha= 255 );

	void R8_GetMip( const unsigned char* in, unsigned char* out, int width, int height );
	void RGBA8_GetMip( const unsigned char* in, unsigned char* out, int width, int height );
	void RGBA8_GetMip2( const unsigned char* in, unsigned char* out, int width, int height );

	void RGBA8_MirrorVertical( unsigned char* in_out, int width, int height );
	void RGBA8_MirrorVerticalAndSwapRB( unsigned char* in_out, int width, int height );

	// Loads texture, converts to supported format, build mips, setup trilinear filtration.
	// Input - empty not created texture. Output - texture with some data and binded to current texture unit.
	// If cannot load texture or invalid format of input image stub texture generated..
	void LoadTexture( r_Texture* texture, const char* filename, unsigned int lod= 0 );
}

