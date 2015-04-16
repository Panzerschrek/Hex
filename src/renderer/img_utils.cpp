#include "img_utils.hpp"

#include <QImage>
#include "framebuffer_texture.hpp"

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


void r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( unsigned char* in_out, int width, int height )
{
	for( int y= 0; y< height>>1; y++ )
		for( int x= 0,
				k= y * width,
				k2= ( height - y - 1 ) * width
					; x< width; x++, k++, k2++ )
		{
			int* p = (int*) in_out;
			int c= p[k];
			p[k]= p[k2];
			p[k2]= c;
			/*unsigned char tmp[4];
			tmp[0]= in_out[k*4  ];
			tmp[1]= in_out[k*4+1];
			tmp[2]= in_out[k*4+2];
			tmp[3]= in_out[k*4+3];
			in_out[k*4  ]= in_out[k2*4  ];
			in_out[k*4+1]= in_out[k2*4+1];
			in_out[k*4+2]= in_out[k2*4+2];
			in_out[k*4+3]= in_out[k2*4+3];
			in_out[k2*4  ]= tmp[0];
			in_out[k2*4+1]= tmp[1];
			in_out[k2*4+2]= tmp[2];
			in_out[k2*4+3]= tmp[3];*/

			unsigned char tmp= in_out[k*4];
			in_out[k*4]= in_out[k*4+2];
			in_out[k*4+2]= tmp;
			tmp= in_out[k2*4];
			in_out[k2*4]= in_out[k2*4+2];
			in_out[k2*4+2]= tmp;
		}
}


void r_ImgUtils::LoadTexture( r_FramebufferTexture* texture, const char* filename )
{
	auto make_stub = [] ( r_FramebufferTexture* texture )
	{
		const constexpr unsigned int stub_tex_size= 32;
		unsigned char data[ stub_tex_size * stub_tex_size * 4 ];

		for( int y= 0; y< stub_tex_size; y++ )
			for( int x= 0; x< stub_tex_size; x++ )
			{
				int ind= 4 * (x + y*stub_tex_size);
				data[ind  ]=
					data[ind+1]=
						data[ind+2]= 0;
				data[ind+3]= 255;
			}
		for( int y= 0; y< stub_tex_size/2; y++ )
			for( int x= 0; x< stub_tex_size/2; x++ )
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
		texture->Create( r_FramebufferTexture::FORMAT_RGBA8, stub_tex_size, stub_tex_size, data );
	};

	QImage img( filename );
	if ( img.isNull() )
	{
		make_stub(texture);
		goto after_creating;
	}


	if( img.depth() == 1 )
	{
		unsigned char* tmp_data= new unsigned char[ img.width() * img.height() ];
		R1_To_R8( img.constBits(), tmp_data, img.width(), img.height() );
		texture->Create( r_FramebufferTexture::FORMAT_R8, img.width(), img.height(), tmp_data );
		delete[] tmp_data;
	}
	else if( img.depth() == 8 )
		texture->Create( r_FramebufferTexture::FORMAT_R8, img.width(), img.height(), img.constBits() );
	else if( img.depth() == 32 )
	{
		unsigned char* img_data= (unsigned char*)img.constBits();
		RGBA8_MirrorVerticalAndSwapRB( img_data, img.width(), img.height() );
		texture->Create( r_FramebufferTexture::FORMAT_RGBA8, img.width(), img.height(), img_data );
	}
	else
		make_stub(texture);

after_creating:

	texture->SetFiltration( r_FramebufferTexture::FILTRATION_LINEAR_MIPMAP_LINEAR, r_FramebufferTexture::FILTRATION_LINEAR );
	texture->BuildMips();
}
