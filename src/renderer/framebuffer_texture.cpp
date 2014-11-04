#include "framebuffer_texture.hpp"



GLenum r_FramebufferTexture::FormatToInternalFormat( TextureFormat format )
{
	switch(format)
	{
		// 1 component
		case FORMAT_R8:
			return GL_R8;
		case FORMAT_R8S:
			return GL_R8_SNORM;
		case FORMAT_R8UI:
			return GL_R8UI;
		case FORMAT_R8I:
			return GL_R8I;

		case FORMAT_R16:
			return GL_R16;
		case FORMAT_R16S:
			return GL_R16_SNORM;
		case FORMAT_R16UI:
			return GL_R16UI;
		case FORMAT_R16I:
			return GL_R16I;
		case FORMAT_R16F:
			return GL_R16F;

		case FORMAT_R32UI:
			return GL_R32UI;
		case FORMAT_R32I:
			return GL_R32I;
		case FORMAT_R32F:
			return GL_R32F;


		// 2 component
		case FORMAT_RG8:
			return GL_RG8;
		case FORMAT_RG8S:
			return GL_RG8_SNORM;
		case FORMAT_RG8UI:
			return GL_RG8UI;
		case FORMAT_RG8I:
			return GL_RG8I;

		case FORMAT_RG16:
			return GL_RG16;
		case FORMAT_RG16S:
			return GL_RG16_SNORM;
		case FORMAT_RG16UI:
			return GL_RG16UI;
		case FORMAT_RG16I:
			return GL_RG16I;
		case FORMAT_RG16F:
			return GL_RG16F;

		case FORMAT_RG32UI:
			return GL_RG32UI;
		case FORMAT_RG32I:
			return GL_RG32I;
		case FORMAT_RG32F:
			return GL_RG32F;


		// 4 component
		case FORMAT_RGBA8:
			return GL_RGBA8;
		case FORMAT_RGBA8S:
			return GL_RGBA8_SNORM;
		case FORMAT_RGBA8UI:
			return GL_RGBA8UI;
		case FORMAT_RGBA8I:
			return GL_RGBA8I;

		case FORMAT_RGBA16:
			return GL_RGBA16;
		case FORMAT_RGBA16S:
			return GL_RGBA16_SNORM;
		case FORMAT_RGBA16UI:
			return GL_RGBA16UI;
		case FORMAT_RGBA16I:
			return GL_RGBA16I;
		case FORMAT_RGBA16F:
			return GL_RGBA16F;

		case FORMAT_RGBA32UI:
			return GL_RGBA32UI;
		case FORMAT_RGBA32I:
			return GL_RGBA32I;
		case FORMAT_RGBA32F:
			return GL_RGBA32F;


		case FORMAT_DEPTH16:
			return GL_DEPTH_COMPONENT16;
		case FORMAT_DEPTH32:
			return GL_DEPTH_COMPONENT32;
		case FORMAT_DEPTH32F:
			return GL_DEPTH_COMPONENT32F;
		case FORMAT_DEPTH24_STENCIL8:
			return GL_DEPTH24_STENCIL8;

		default:
			return 0;
	};
}

GLenum r_FramebufferTexture::FormatToBaseFormat( TextureFormat format )
{
	switch(format)
	{
		// 1 component
		case FORMAT_R8:
		case FORMAT_R8S:
		case FORMAT_R8UI:
		case FORMAT_R8I:

		case FORMAT_R16:
		case FORMAT_R16S:
		case FORMAT_R16UI:
		case FORMAT_R16I:
		case FORMAT_R16F:

		case FORMAT_R32UI:
		case FORMAT_R32I:
		case FORMAT_R32F:
			return GL_RED;

		// 2 component
		case FORMAT_RG8:
		case FORMAT_RG8S:
		case FORMAT_RG8UI:
		case FORMAT_RG8I:

		case FORMAT_RG16:
		case FORMAT_RG16S:
		case FORMAT_RG16UI:
		case FORMAT_RG16I:
		case FORMAT_RG16F:

		case FORMAT_RG32UI:
		case FORMAT_RG32I:
		case FORMAT_RG32F:
			return GL_RG;


		// 4 component
		case FORMAT_RGBA8:
		case FORMAT_RGBA8S:
		case FORMAT_RGBA8UI:
		case FORMAT_RGBA8I:

		case FORMAT_RGBA16:
		case FORMAT_RGBA16S:
		case FORMAT_RGBA16UI:
		case FORMAT_RGBA16I:
		case FORMAT_RGBA16F:

		case FORMAT_RGBA32UI:
		case FORMAT_RGBA32I:
		case FORMAT_RGBA32F:
			return GL_RGBA;


		case FORMAT_DEPTH16:
		case FORMAT_DEPTH32:
		case FORMAT_DEPTH32F:
			return GL_DEPTH_COMPONENT;
		case FORMAT_DEPTH24_STENCIL8:
			return GL_DEPTH_STENCIL;

		default:
			return 0;
	};
}


GLenum r_FramebufferTexture::FiltrationToGLFiltration( TextureFiltration filtration )
{
	switch(filtration)
	{
		case FILTRATION_NEAREST:
			return GL_NEAREST;
		case FILTRATION_LINEAR:
			return GL_LINEAR;
		case FILTRATION_NEAREST_MIPMAP_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;
		case FILTRATION_NEAREST_MIPMAP_LINEAR:
			return GL_NEAREST_MIPMAP_LINEAR;
		case FILTRATION_LINEAR_MIPMAP_NEAREST:
			return GL_LINEAR_MIPMAP_NEAREST;
		case FILTRATION_LINEAR_MIPMAP_LINEAR:
			return GL_LINEAR_MIPMAP_LINEAR;

		default:
			return 0;
	};
}

GLenum r_FramebufferTexture::WrapModeToGLWrapMode( TextureWrapMode mode )
{
	switch(mode)
	{
		case WRAP_MODE_CLAMP:
			return GL_CLAMP_TO_EDGE;
		case WRAP_MODE_REPEAT:
			return GL_REPEAT;

		default:
			return 0;
	};
}

r_FramebufferTexture::r_FramebufferTexture()
	: created(false)
	, wrap_mode( WRAP_MODE_REPEAT )
	, filter_mag( FILTRATION_LINEAR )
	, filter_min( FILTRATION_NEAREST_MIPMAP_LINEAR )
	, size_x(0), size_y(0)
{
}

r_FramebufferTexture::~r_FramebufferTexture()
{
}

void r_FramebufferTexture::Create( TextureFormat f, unsigned int width, unsigned int height )
{
	size_x= width;
	size_y= height;
	format= f;

	glGenTextures( 1, &tex_id );
	glBindTexture( GL_TEXTURE_2D, tex_id );

	SetData<unsigned char>( NULL );

	created= true;
}

void r_FramebufferTexture::Create( TextureFormat f, unsigned int width, unsigned int height, const unsigned char* data )
{
	size_x= width;
	size_y= height;
	format= f;

	glGenTextures( 1, &tex_id );
	glBindTexture( GL_TEXTURE_2D, tex_id );

	SetData<unsigned char>( data );

	created= true;
}

void r_FramebufferTexture::Create( TextureFormat f, unsigned int width, unsigned int height, const float* data )
{
	size_x= width;
	size_y= height;
	format= f;

	glGenTextures( 1, &tex_id );
	glBindTexture( GL_TEXTURE_2D, tex_id );

	SetData<float>( data );

	created= true;
}

void r_FramebufferTexture::Destroy()
{
	glDeleteTextures( 1, &tex_id );

	created= false;
}

void r_FramebufferTexture::Bind( unsigned int unit ) const
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_2D, tex_id );
}

void r_FramebufferTexture::SetWrapMode( TextureWrapMode mode )
{
	if( wrap_mode != mode )
	{
		wrap_mode= mode;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, WrapModeToGLWrapMode(wrap_mode) );
	}
}

void r_FramebufferTexture::SetFiltration( TextureFiltration f_min, TextureFiltration f_mag )
{
	if( f_min != filter_min )
	{
		filter_min= f_min;
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FiltrationToGLFiltration(filter_min) );
	}
	if( f_mag != filter_mag )
	{
		filter_mag= f_mag;
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FiltrationToGLFiltration(filter_min) );
	}
}


void r_FramebufferTexture::BuildMips()
{
	glGenerateMipmap( GL_TEXTURE_2D );
}


bool r_FramebufferTexture::IsDepthTexture() const
{
	return (
			format == FORMAT_DEPTH16  ||
			format == FORMAT_DEPTH32  ||
			format == FORMAT_DEPTH32F ||
			format == FORMAT_DEPTH24_STENCIL8 );
}
