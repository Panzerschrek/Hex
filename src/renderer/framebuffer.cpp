#include "framebuffer.hpp"


unsigned int r_Framebuffer::screen_framebuffer_width_= 0, r_Framebuffer::screen_framebuffer_height_= 0;
r_Framebuffer* r_Framebuffer::current_framebuffer_= nullptr;

r_Framebuffer::r_Framebuffer()
	: framebuffer_id_(0)
	, size_x_(0), size_y_(0)
{
}

r_Framebuffer::~r_Framebuffer()
{
}

void r_Framebuffer::Create( const std::vector< r_FramebufferTexture::TextureFormat >& color_textures,
				 r_FramebufferTexture::TextureFormat depth_buffer_texture_format,
				unsigned int width, unsigned int height )
{
	size_x_= width;
	size_y_= height;

	glGenFramebuffers( 1, &framebuffer_id_ );
    glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id_ );

    textures_.resize( color_textures.size() );

	GLenum color_attachments[32];
    for( int i= 0; i< textures_.size(); i++ )
    {
		textures_[i].Create( color_textures[i], size_x_, size_y_ );
		textures_[i].SetFiltration( r_FramebufferTexture::FILTRATION_NEAREST, r_FramebufferTexture::FILTRATION_NEAREST );
		textures_[i].SetWrapMode( r_FramebufferTexture::WRAP_MODE_CLAMP );

		glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures_[i].tex_id, 0 );
		color_attachments[ i ]= GL_COLOR_ATTACHMENT0 + i;

	}// for textures
	glDrawBuffers( textures_.size(), color_attachments );

	if( depth_buffer_texture_format != r_FramebufferTexture::FORMAT_UNKNOWN )
	{
		depth_texture_.Create( depth_buffer_texture_format, size_x_, size_y_ );
		depth_texture_.SetFiltration( r_FramebufferTexture::FILTRATION_NEAREST, r_FramebufferTexture::FILTRATION_NEAREST );
		depth_texture_.SetWrapMode( r_FramebufferTexture::WRAP_MODE_CLAMP );

		GLenum attach_type= depth_buffer_texture_format == r_FramebufferTexture::FORMAT_DEPTH24_STENCIL8 ?
				GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
		glFramebufferTexture( GL_FRAMEBUFFER, attach_type, depth_texture_.tex_id, 0 );
	}


	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void r_Framebuffer::Destroy()
{
	for( int i= 0; i < textures_.size(); i++ )
		textures_[i].Destroy();

	if( depth_texture_.Created() )
		depth_texture_.Destroy();


//	glDeleteFramebuffers( 1, &framebuffer_id );
}


void r_Framebuffer::Bind()
{
	current_framebuffer_= this;
	glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id_ );
	glViewport( 0, 0, size_x_, size_y_ );
}

void r_Framebuffer::BindScreenFramebuffer()
{
	current_framebuffer_= nullptr;
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, screen_framebuffer_width_, screen_framebuffer_height_ );
}

