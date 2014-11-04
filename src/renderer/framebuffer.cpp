#include "framebuffer.hpp"

r_Framebuffer::r_Framebuffer()
	: framebuffer_id(0)
	, size_x(0), size_y(0)
{
}

r_Framebuffer::~r_Framebuffer()
{
}


void r_Framebuffer::Create( const std::vector< r_FramebufferTexture::TextureFormat >& color_textures,
				 r_FramebufferTexture::TextureFormat depth_buffer_texture_format,
				unsigned int width, unsigned int height )
{
	size_x= width;
	size_y= height;

	glGenFramebuffers( 1, &framebuffer_id );
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id );

    textures.resize( color_textures.size() );

	GLenum color_attachments[32];
    for( int i= 0; i< textures.size(); i++ )
    {
		textures[i].Create( color_textures[i], size_x, size_y );
		textures[i].SetFiltration( r_FramebufferTexture::FILTRATION_NEAREST, r_FramebufferTexture::FILTRATION_NEAREST );
		textures[i].SetWrapMode( r_FramebufferTexture::WRAP_MODE_CLAMP );

		glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures[i].tex_id, 0 );
		color_attachments[ i ]= GL_COLOR_ATTACHMENT0 + i;

	}// for textures
	glDrawBuffers( textures.size(), color_attachments );

	if( depth_buffer_texture_format != r_FramebufferTexture::FORMAT_UNKNOWN )
	{
		depth_texture.Create( depth_buffer_texture_format, size_x, size_y );
		depth_texture.SetFiltration( r_FramebufferTexture::FILTRATION_NEAREST, r_FramebufferTexture::FILTRATION_NEAREST );
		depth_texture.SetWrapMode( r_FramebufferTexture::WRAP_MODE_CLAMP );

		GLenum attach_type= depth_buffer_texture_format == r_FramebufferTexture::FORMAT_DEPTH24_STENCIL8 ?
				GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		glFramebufferTexture( GL_FRAMEBUFFER, attach_type, depth_texture.tex_id, 0 );
	}


	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void r_Framebuffer::Destroy()
{
	for( int i= 0; i < textures.size(); i++ )
		textures[i].Destroy();

	if( depth_texture.Created() )
		depth_texture.Destroy();
}


void r_Framebuffer::Bind()
{
	 glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id );
}

