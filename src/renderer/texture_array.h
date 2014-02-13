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
#ifndef TEXTURE_ARRAY_H
#define TEXTURE_ARRAY_H
#include "ph.h"

class r_TextureArray
{
    public:
    r_TextureArray();
    ~r_TextureArray();

    int MoveOnGPU();
    void DeleteFromGPU();
    void SetFiltration(GLuint min, GLuint mag );

    void TextureData( int w, int h, int l, GLuint d_type, GLuint t_type, int bpp, void* data );
    void TextureLayer( int layer, unsigned char* data );
    void DeleteFromRAM();
    void GenerateMipmap();
    void Bind( int unit= 0 );

    private:
    GLuint id;

    bool in_video_memory;// текстура есть в видеопамяти
	bool in_ram;// текстура есть в оперативке
	int bits_per_pixel;
	int width;
	int height;
	int layers;
	unsigned char* texture_data;
	GLuint texture_type, data_type;
};

#endif//TEXTURE_ARRAY_H
