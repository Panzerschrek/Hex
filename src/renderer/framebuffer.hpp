#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <vector>

#include "ph.h"
#include "framebuffer_texture.hpp"


class r_Framebuffer
{
public:

	r_Framebuffer();
	~r_Framebuffer();

	void Create( const std::vector< r_FramebufferTexture::TextureFormat >& color_textures,
				 r_FramebufferTexture::TextureFormat depth_buffer_texture_format,
				unsigned int width, unsigned int height );
	void Destroy();

	void Bind();

	unsigned int Width() const;
	unsigned int Height() const;

	std::vector< r_FramebufferTexture > * GetTextures();
	r_FramebufferTexture* GetDepthTexture();

	static unsigned int CurrentFramebufferWidth();
	static unsigned int CurrentFramebufferHeight();
	static void SetScreenFramebufferSize( int width, int height );
	static void BindScreenFramebuffer();
private:

	GLuint framebuffer_id_;
	unsigned int size_x_, size_y_;

	std::vector<r_FramebufferTexture> textures_;
	r_FramebufferTexture depth_texture_;

	static unsigned int screen_framebuffer_width_, screen_framebuffer_height_;
	static r_Framebuffer* current_framebuffer_;
};



inline std::vector< r_FramebufferTexture > * r_Framebuffer::GetTextures()
{
	return &textures_;
}

inline r_FramebufferTexture * r_Framebuffer::GetDepthTexture()
{
	return &depth_texture_;
}

inline unsigned int r_Framebuffer::CurrentFramebufferWidth()
{
	return current_framebuffer_ == nullptr ? screen_framebuffer_width_ : current_framebuffer_->size_x_;
}
inline unsigned int r_Framebuffer::CurrentFramebufferHeight()
{
	return current_framebuffer_ == nullptr ? screen_framebuffer_height_ : current_framebuffer_->size_y_;
}

inline unsigned int r_Framebuffer::Width() const
{
	return size_x_;
}
inline unsigned int r_Framebuffer::Height() const
{
	return size_y_;
}

inline void r_Framebuffer::SetScreenFramebufferSize( int width, int height )
{
	screen_framebuffer_width_= width;
	screen_framebuffer_height_= height;
}


#endif//FRAMEBUFFER_HPP
