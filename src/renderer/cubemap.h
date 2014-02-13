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
#ifndef CUBEMAP_H
#define CUBEMAP_H
#include "ph.h"

#include "texture.h"

class r_CubeMap
{
    public:
    void  Create();
    int MoveOnGPU();
    int TextureData( int number, int w, int h, GLuint d_type, GLuint t_type, int bpp, void* data );
    void SetFiltration( GLuint min, GLuint mag );
    void DeleteFromGPU();
    void DeleteFromRAM();
    int Load( const char* file_name_0 );
    void Bind( unsigned int unit= 0 );

    r_CubeMap();
    ~r_CubeMap();
    private:


    GLuint id;
    unsigned char* texture_data[6];
    int width[6];
	int height[6];

    GLuint texture_type[6];
    GLuint data_type[6];
    int bits_per_pixel[6];

    bool created;
    bool in_video_memory;// текстура есть в видеопамяти
	bool in_ram;// текстура есть в оперативке
	
    GLuint filtration_min, filtration_mag;


};

#endif//CUBEMAP_H
