#ifndef IMG_UTILS_HPP
#define IMG_UTILS_HPP

class r_FramebufferTexture;

class r_ImgUtils
{
public:

	static void RGBA8_To_BRGA8( const unsigned char* in, unsigned char* out, int width, int height );
	static void RGBA8_To_BRGA8( unsigned char* in_out, int width, int height );
	static void R1_To_R8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char white_value= 255 );
	static void R8_To_RGBA8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char alpha= 255 );


	static void RGBA8_GetMip( const unsigned char* in, unsigned char* out, int width, int height );
	static void RGBA8_GetMip2( const unsigned char* in, unsigned char* out, int width, int height );

	static void RGBA8_MirrorVerticalAndSwapRB( unsigned char* in_out, int width, int height );


	// loads texture, converts to supported format, build mips, setup trilinear filtration
	// input - empty not created texture. Output - texture with some data and binded to current texture unit
	// if cannot load texture or invalid format of input image stub texture generated
	static void LoadTexture( r_FramebufferTexture* texture, const char* filename );
};

#endif
