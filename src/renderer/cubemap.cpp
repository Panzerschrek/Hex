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
#ifndef CUBEMAP_CPP
#define CUBEMAP_CPP
#include "ph.h"
#include "cubemap.h"


int r_CubeMap::Load( const char* file_name_0 )
{
    unsigned int i= 0;
    unsigned int len= strlen( file_name_0 );
    char fn[128];
    strcpy( fn, file_name_0 );
    r_TextureFile tf;

    for( i =0; i< 6; i++ )
    {
        fn[ len - 5 ]= '0' + i;

        if( !strcmp( fn + len - 3, "bmp") || !strcmp( fn + len - 3, "BMP") )
        {
            if ( rLoadTextureBMP( &tf, fn ) )
                rDefaultTexture( &tf );
        }
        else if( !strcmp( fn + len - 3, "jpg") || !strcmp( fn + len - 3, "JPG")  )
        {
            //if (rLoadTextureJPG( &tf, fn ) )
                rDefaultTexture( &tf );
        }
        else if( !strcmp( fn + len - 3, "tga") || !strcmp( fn + len - 3, "TGA") )
        {
            if ( rLoadTextureTGA( &tf, fn ) )
                rDefaultTexture( &tf );
        }
        else
            rDefaultTexture( &tf );

        TextureData( i, tf.width, tf.height, tf.data_type, tf.texture_type, tf.bits_per_pixel, tf.data );

    }
    Create();
    MoveOnGPU();
    return 0;
}

int r_CubeMap::TextureData( int number, int w, int h, GLuint d_type, GLuint t_type, int bpp, void* data )
{
    if (number > 5 || number < 0 )
        return 1;

    if( texture_data[number] != NULL )
        delete[] texture_data[number];

    width[number]= w;
    height[number]= h;
    data_type[number]= d_type;
    texture_type[number]= t_type;
    bits_per_pixel[number]= bpp;

    texture_data[number]= (unsigned char*) data;

    in_ram= true;
    return 0;
}

void r_CubeMap::Create()
{
    glGenTextures( 1, &id );
	created= true;
}

void r_CubeMap::Bind( unsigned int unit )
{
    glActiveTexture( unit + GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, id );
}

int r_CubeMap::MoveOnGPU()
{
    unsigned int i;
    if(created)
	{
	    Bind();
	    glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );



		#ifdef OGL21//ogl 2.1 mipmaps autogeneration
		if( filtration_min == GL_LINEAR_MIPMAP_LINEAR ||filtration_min == GL_LINEAR_MIPMAP_NEAREST
		 || filtration_min == GL_NEAREST_MIPMAP_LINEAR || filtration_min == GL_NEAREST_MIPMAP_NEAREST )
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE );
		#endif

	    for( i= 0; i< 6; i++ )
	    {
	        glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, texture_type[i], width[i], height[i],
							0, texture_type[i], data_type[i], texture_data[i] );

	    }
	   #ifndef OGL21//ogl3 mipmap generation
		if( filtration_min == GL_LINEAR_MIPMAP_LINEAR || filtration_min == GL_LINEAR_MIPMAP_NEAREST
		 || filtration_min == GL_NEAREST_MIPMAP_LINEAR || filtration_min == GL_NEAREST_MIPMAP_NEAREST )
		glGenerateMipmap( GL_TEXTURE_CUBE_MAP );
	  #endif
	    in_video_memory= true;
	}
	return 0;
}

void r_CubeMap::SetFiltration( GLuint min, GLuint mag )
{
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, min );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, mag );
	filtration_min= min;
filtration_mag= mag;
}

void r_CubeMap::DeleteFromGPU()
{
    glDeleteTextures( 1, &id );
    in_video_memory= false;
    created= false;
}
void r_CubeMap::DeleteFromRAM()
{
    register unsigned int i= 0;
    for( i= 0; i< 6; i++ )
    {
        if( texture_data[i] != NULL )
            delete[] texture_data[i];
    }
    in_ram= false;
}

r_CubeMap::r_CubeMap()
{
    created= false;
    in_ram= false;
    in_video_memory= false;

filtration_min= filtration_mag= GL_NEAREST;
    unsigned int i;
    for( i= 0; i< 6; i++ )
        texture_data[i]= NULL;

}

r_CubeMap::~r_CubeMap()
{
    DeleteFromRAM();
    DeleteFromGPU();

}
#endif//CUBEMAP_CPP
