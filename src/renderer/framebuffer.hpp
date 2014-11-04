#ifndef FRAMEBUFFER_HPP
#define FRAMEBIFFER_HPP

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

	const std::vector< r_FramebufferTexture > * GetTextures() const;
	const r_FramebufferTexture* GetDepthTexture() const;

private:

	GLuint framebuffer_id;
	unsigned int size_x, size_y;

	std::vector<r_FramebufferTexture> textures;
	r_FramebufferTexture depth_texture;
};



inline const std::vector< r_FramebufferTexture > * r_Framebuffer::GetTextures() const
{
	return &textures;
}

inline const r_FramebufferTexture * r_Framebuffer::GetDepthTexture() const
{
	return &depth_texture;
}

#endif//FRAMEBIFFER_HPP
