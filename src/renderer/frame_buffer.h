#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "ph.h"
struct r_TextureType
{
	int number_of_components;
	int data_type;
};

#define R_MAX_FRAMEBUFFER_COLOR_TEXTURE 4
class r_FrameBuffer
{
public:
	int Create( int depth_bits, int stencil_bits, int color_textures_number, unsigned int* color_tex_components, GLuint* colr_tex_data_types, int size_x, int size_y );
	void ClearBuffer( bool clear_depth, bool clear_color );

	 r_FrameBuffer();
	 ~r_FrameBuffer();
	 void Bind()const;
	 void BindColorTexture( unsigned int number, int unit );
	 void BindDepthTexture( unsigned int unit );

	 void SetDepthTextureFiltration( unsigned int min, unsigned int mag );
	 void SetColorTextureFiltration( unsigned int number, unsigned int min, unsigned int mag );

	 void SetDepthBuffer( r_FrameBuffer* depth_buffer );
	 void DeattachDepthTexture();
	 void ReattachDepthTexture();

	 void DesableDepthTextureCompareMode();
	 static void BindNull();
private:
	static r_FrameBuffer* current_buffer;
	GLuint id;
	GLuint depth_texture_id;
	GLuint color_textures_id[ R_MAX_FRAMEBUFFER_COLOR_TEXTURE ];
	unsigned int number_of_color_textures;
	bool is_depth_texture;
	bool is_stencil;
	bool created;
	int x_size, y_size;
	unsigned int depth_bits;
};


#endif//FRAME_BUFFER_H
