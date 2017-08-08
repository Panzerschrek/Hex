#include <cstring>

// This is single place, where this library included.
// So, we can include implementation here.
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

#include "texture.hpp"
#include "../console.hpp"

#include "img_utils.hpp"

static void MakeStub( r_Texture* texture )
{
	const constexpr unsigned int stub_tex_size= 32;
	unsigned char data[ stub_tex_size * stub_tex_size * 4 ];

	for( unsigned int y= 0; y< stub_tex_size; y++ )
		for( unsigned int x= 0; x< stub_tex_size; x++ )
		{
			int ind= 4 * (x + y*stub_tex_size);
			data[ind  ]=
			data[ind+1]=
			data[ind+2]= 0;
			data[ind+3]= 255;
		}
	for( unsigned int y= 0; y< stub_tex_size/2; y++ )
		for( unsigned int x= 0; x< stub_tex_size/2; x++ )
		{
			int ind= 4 * (x + y*stub_tex_size);
			data[ind  ]= 240;
			data[ind+1]= 12;
			data[ind+2]= 245;
			ind= 4 * (x+stub_tex_size/2 + (y+stub_tex_size/2)*stub_tex_size);
			data[ind  ]= 240;
			data[ind+1]= 12;
			data[ind+2]= 245;
		}
	*texture= r_Texture( r_Texture::PixelFormat::RGBA8, stub_tex_size, stub_tex_size, data );
}

void r_ImgUtils::RGBA8_To_BRGA8( const unsigned char* in, unsigned char* out, int width, int height )
{
	for( int i= 0, i_end= width * height * 4; i< i_end; i+= 4 )
	{
		out[i  ]= in[i+2];
		out[i+1]= in[i+1];
		out[i+2]= in[i  ];
		out[i+3]= in[i+3];
	}
}

void r_ImgUtils::RGBA8_To_BRGA8( unsigned char* in_out, int width, int height )
{
	for( int i= 0, i_end= width * height * 4; i< i_end; i+= 4 )
	{
		unsigned char tmp= in_out[i];
		in_out[i]= in_out[i+2];
		in_out[i+2]= tmp;
	}
}

void r_ImgUtils::R1_To_R8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char white_value )
{
	for( int i= 0, i_end= width * height * 4; i< i_end; i+= 4 )
	{
		out[i]= ( in[i>>3] & (1<<(i&7)) ) == 0 ? 0 : white_value;
	}
}

void r_ImgUtils::R8_To_RGBA8( const unsigned char* in, unsigned char* out, int width, int height, unsigned char alpha )
{
	for( int i= 0, i_end= width * height; i< i_end; i++ )
	{
		int j= i<<2;
		out[j  ]=
			out[j+1]=
				out[j+2]= in[i];
		out[j+3]= alpha;
	}
}

void r_ImgUtils::R8_GetMip( const unsigned char* in, unsigned char* out, int width, int height )
{
	unsigned char* dst= out;
	for( int y= 0; y< height; y+= 2 )
	{
		const unsigned char* src[2]= { in + y * width, in + (y+1) * width };
		for( int x= 0; x< width; x+= 2, src[0]+= 2, src[1]+= 2, dst++ )
			dst[0]= ( src[0][0] + src[0][1] + src[1][0] + src[1][1] ) >> 2;
	}
}

void r_ImgUtils::RGBA8_GetMip( const unsigned char* in, unsigned char* out, int width, int height )
{
	unsigned char* dst= out;
	for( int y= 0; y< height; y+= 2 )
	{
		const unsigned char* src[2]= { in + y * width * 4, in + (y+1) * width * 4 };
		for( int x= 0; x< width; x+= 2, src[0]+= 8, src[1]+= 8, dst+= 4 )
		{
			dst[0]= ( src[0][0] + src[0][4]  +  src[1][0] + src[1][4] )>>2;
			dst[1]= ( src[0][1] + src[0][5]  +  src[1][1] + src[1][5] )>>2;
			dst[2]= ( src[0][2] + src[0][6]  +  src[1][2] + src[1][6] )>>2;
			dst[3]= ( src[0][3] + src[0][7]  +  src[1][3] + src[1][7] )>>2;
		}
	}
}

void r_ImgUtils::RGBA8_GetMip2( const unsigned char* in, unsigned char* out, int width, int height )
{
	unsigned char* dst= out;
	for( int y= 0; y< height; y+= 4 )
	{
		const unsigned char* src[4]= {
			in + y * width * 4,
			in + (y+1) * width * 4,
			in + (y+2) * width * 4,
			in + (y+3) * width * 4
		};
		for( int x= 0; x< width; x+= 4, src[0]+= 16, src[1]+= 16, dst+= 4 )
		{
			dst[0]= (   src[0][0] + src[0][4] + src[0][8 ] + src[0][12] +
						src[1][0] + src[1][4] + src[1][8 ] + src[1][12] +
						src[2][0] + src[2][4] + src[2][8 ] + src[2][12] +
						src[3][0] + src[3][4] + src[3][8 ] + src[3][12] )>>4;
			dst[1]= (   src[0][1] + src[0][5] + src[0][9 ] + src[0][13] +
						src[1][1] + src[1][5] + src[1][9 ] + src[1][13] +
						src[2][1] + src[2][5] + src[2][9 ] + src[2][13] +
						src[3][1] + src[3][5] + src[3][9 ] + src[3][13] )>>4;
			dst[2]= (   src[0][2] + src[0][6] + src[0][10] + src[0][14] +
						src[1][2] + src[1][6] + src[1][10] + src[1][14] +
						src[2][2] + src[2][6] + src[2][10] + src[2][14] +
						src[3][2] + src[3][6] + src[3][10] + src[3][14] )>>4;
			dst[3]= (   src[0][3] + src[0][7] + src[0][11] + src[0][15] +
						src[1][3] + src[1][7] + src[1][11] + src[1][15] +
						src[2][3] + src[2][7] + src[2][11] + src[2][15] +
						src[3][3] + src[3][7] + src[3][11] + src[3][15] )>>4;
		}
	}
}

void r_ImgUtils::RGBA8_MirrorVertical( unsigned char* in_out, int width, int height )
{
	for( int y= 0; y < (height >> 1); y++ )
		for( int x= 0,
				k= y * width,
				k2= ( height - y - 1 ) * width
					; x< width; x++, k++, k2++ )
		{
			int* p = (int*) in_out;
			int c= p[k];
			p[k]= p[k2];
			p[k2]= c;
		}
}

void r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( unsigned char* in_out, int width, int height )
{
	for( int y= 0; y < (height >> 1); y++ )
		for( int x= 0,
				k= y * width,
				k2= ( height - y - 1 ) * width
					; x< width; x++, k++, k2++ )
		{
			int* p = (int*) in_out;
			int c= p[k];
			p[k]= p[k2];
			p[k2]= c;

			unsigned char tmp= in_out[k*4];
			in_out[k*4]= in_out[k*4+2];
			in_out[k*4+2]= tmp;
			tmp= in_out[k2*4];
			in_out[k2*4]= in_out[k2*4+2];
			in_out[k2*4+2]= tmp;
		}
}

void r_ImgUtils::LoadTexture( r_Texture* texture, const char* filename, unsigned int lod )
{
	const unsigned int c_min_texture_size= 4;

	int width, height;
	int channels= 0;
	unsigned char* stbi_img_data = stbi_load(filename, &width, &height, &channels, 0 );

	unsigned char* data= stbi_img_data;

	if ( data == nullptr )
	{
		h_Console::Warning( "Can not load texture: \"", filename, "\"" );
		MakeStub(texture);
		goto after_creating;
	}

	if( width < int(c_min_texture_size) || height < int(c_min_texture_size) )
	{
		stbi_image_free( stbi_img_data );
		h_Console::Warning( "Texture is too small: \"", filename, "\"" );
		MakeStub(texture);
		goto after_creating;
	}

	if( channels == 1 )
	{
		if( lod == 0 )
			*texture= r_Texture( r_Texture::PixelFormat::R8, width, height, data );
		else
		{
			std::vector<unsigned char> tmp_lod_data( ( width / 2 ) * ( height / 2 ) );
			unsigned char* d[2]= { data, tmp_lod_data.data() };

			unsigned int l= 0, lod_w= width, lod_h= height;
			for( ;
				l < lod && lod_w >= c_min_texture_size && lod_h >= c_min_texture_size;
				l++, lod_w >>= 1, lod_h >>= 1 )
				R8_GetMip( d[ l&1 ], d[ (l^1)&1 ], lod_w, lod_h );

			*texture= r_Texture( r_Texture::PixelFormat::R8, lod_w, lod_h, d[ l&1 ] );
		}
	}
	else if( channels == 3 || channels == 4 )
	{
		std::vector<unsigned char> data_rgba;
		if( channels == 3 )
		{
			data_rgba.resize( width * height * 4u );
			for( unsigned int i= 0u; i < static_cast<unsigned int>( width * height ); i++ )
			{
				data_rgba[i*4u+0u]= data[i*3u+0u];
				data_rgba[i*4u+1u]= data[i*3u+1u];
				data_rgba[i*4u+2u]= data[i*3u+2u];
				data_rgba[i*4u+3u]= 255u;
			}
			data= data_rgba.data();
		}

		RGBA8_MirrorVertical( data, width, height );

		if( lod == 0 )
			*texture= r_Texture( r_Texture::PixelFormat::RGBA8, width, height, data );
		else
		{
			std::vector<unsigned char> tmp_data( 4 * ( width / 2 ) * ( height / 2 ) );
			unsigned char* d[2]= { data, tmp_data.data() };

			unsigned int l= 0, lod_w= width, lod_h= height;
			for( ;
				l < lod && lod_w >= c_min_texture_size && lod_h >= c_min_texture_size;
				l++, lod_w >>= 1, lod_h >>= 1 )
				RGBA8_GetMip( d[ l&1 ], d[ (l^1)&1 ], lod_w, lod_h );

			*texture= r_Texture( r_Texture::PixelFormat::RGBA8, lod_w, lod_h, d[ l&1 ] );
		}
	}
	else
	{
		h_Console::Warning( "Can not convert texture: \"", filename, "\". Unsupported image depth: ", channels );
		MakeStub(texture);
	}

	stbi_image_free( stbi_img_data );

after_creating:
	texture->SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
	texture->BuildMips();
}

void r_ImgUtils::LoadImageRGBA(
	const char* filename,
	std::vector<unsigned char>& out_data,
	unsigned int& out_width, unsigned int& out_height )
{
	out_data.clear();

	int width, height;
	int channels= 0;
	unsigned char* const stbi_img_data = stbi_load(filename, &width, &height, &channels, 0 );
	if( stbi_img_data == nullptr )
	{
		h_Console::Warning( "Can not load image: \"", filename, "\"" );
		return;
	}

	if( width <= 0 || height <= 0 )
	{
		h_Console::Warning( "Can not load image: \"", filename, "\" - bad size: ", width, "x", height );
		stbi_image_free( stbi_img_data );
		return;
	}
	if( channels > 4 || channels < 3 )
	{
		h_Console::Warning( "Can not load image: \"", filename, "\" - srange channel count: ", channels );
		stbi_image_free( stbi_img_data );
		return;
	}

	out_width = width ;
	out_height= height;

	out_data.resize( out_width * out_height * 4u );

	if( channels == 4 )
		std::memcpy( out_data.data(), stbi_img_data, out_data.size() );
	else
		for( unsigned int i= 0u; i < out_width * out_height; i++ )
		{
			out_data[i*4u+0u]= stbi_img_data[i*3u+0u];
			out_data[i*4u+1u]= stbi_img_data[i*3u+1u];
			out_data[i*4u+2u]= stbi_img_data[i*3u+2u];
			out_data[i*4u+3u]= 255u;
		}

	stbi_image_free( stbi_img_data );

	RGBA8_MirrorVertical( out_data.data(), out_width, out_height );
}
