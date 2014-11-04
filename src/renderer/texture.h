	/*
	*This file is part of FREG.
	*
	*FREG is free software: you can redistribute it and/or modify
	*it under the terms of the GNU General Public License as published by
	*the Free Software Foundation, either version 3 of the License, or
	*(at your option) any later version.
	*
	*FREG is distributed in the hope that it will be useful,
	*but WITHOUT ANY WARRANTY; without even the implied warranty of
	*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	*GNU General Public License for more details.
	*
	*You should have received a copy of the GNU General Public License
	*along with FREG. If not, see <http://www.gnu.org/licenses/>.*/
#ifndef TEXTURE_H
#define TEXTURE_H
#include "ph.h"

 struct r_TextureFile
{
	unsigned char* data;
	int width;
	int height;
	GLuint texture_type;
	GLuint data_type;
	int bits_per_pixel;
	unsigned int data_size;
};


#define r_MAX_TEXTURE_FILENAME_LEN 32
class r_Texture
{
public:
	void BindTexture( int unit = 0) const;
	void Create();
	void MoveOnGPU();
	void TexSubData2GPU( unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char* data );
	void DeleteFromRAM();
	void DeleteFromGPU();
	void SetFiltration( GLuint min, GLuint mag );
	void SetMaxLod( float n );
	void SetMinLod( float n );
	void SetDefaultFiltration();
	void TextureData( int w, int h, GLuint d_type, GLuint t_type, int bpp, void* data );
	int  Load( const char* file );
	r_Texture();
	~r_Texture();

	inline int GetWidth() const
	{
		return width;
	}
	inline int GetHeight() const
	{
		return height;
	}


	static void ResetBinding();

private:

	static r_Texture* current;
	static int current_unit;
	GLuint id;
	GLuint texture_type;//тип - RGB, RGBA
	GLuint data_type;// тип данных - unsigned byte, float

	//int texture_unit;


	bool created;
	bool default_filtration;
	GLuint min_filtration;
	GLuint mag_filtration;

	bool in_video_memory;// текстура есть в видеопамяти
	bool in_ram;// текстура есть в оперативке
	int bits_per_pixel;
	int width;
	int height;
	unsigned char* texture_data;

	char file_name[r_MAX_TEXTURE_FILENAME_LEN];



};


void rRGBAMirrorVerticalAndSwapRB( unsigned char* data, int width, int height );


int rLoadTextureBMP( r_TextureFile* tf, const char* file_name, bool data_is_temp= true );
int rLoadTextureJPG( r_TextureFile* tf, const char* file_name, bool data_is_temp= true );
int rLoadTextureTGA( r_TextureFile* tf, const char* file_name, bool data_is_temp= true );
void rDefaultTexture( r_TextureFile* tf, bool data_is_temp= true );
void rBGRA2RGBA( unsigned char* data, unsigned int size );
void rRGB2RGBAKeyColor( r_TextureFile* tf, unsigned char key_color[3] );
void rRGBAMirrorVertical( r_TextureFile* tf );
void rRGBAMirrorVerticalAndSwapRB( r_TextureFile* tf );
void rRGBAGetMip( r_TextureFile* tf_src, r_TextureFile* tf_dst );
void rRGB_RChanenl2NormalMap(  unsigned char* src_data, unsigned char* dst_data,  unsigned int size_x, unsigned int size_y, float normal_z_size= 1.0f );

int rSaveTextureTGA( const char* file_name, unsigned char* data, unsigned int sizeX, unsigned int sizeY );
#endif//_TEXTURE_H_
