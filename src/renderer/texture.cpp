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
#ifndef TEXTURE_CPP
#define TEXTURE_CPP

#include "texture.h"
#include "jpeglib.h"
#include "../math_lib/vec.h"


r_Texture* r_Texture::current= NULL;
int r_Texture::current_unit= 0;

r_Texture::r_Texture()
{
    in_ram= false;
    in_video_memory= false;
    texture_data= NULL;
    created= false;
    default_filtration= true;
    min_filtration= GL_LINEAR_MIPMAP_LINEAR;
    mag_filtration= GL_LINEAR;
    width= 0;
    height= 0;
    bits_per_pixel= 0;
}

void r_Texture::BindTexture( int unit ) const
{
    //if ( current != this && created )
    //{
    //if( unit != current_unit  )//задаём текущий текстурный юнит
    //{
    glActiveTexture( GL_TEXTURE0 + unit );
    current_unit= unit;
    //}

    glBindTexture( GL_TEXTURE_2D, id );
    current= (r_Texture*) this;
    //}
}


void r_Texture::ResetBinding()
{
    current= NULL;
    current_unit= 0;
    glActiveTexture( GL_TEXTURE0 );
}

void r_Texture::Create()
{
    glGenTextures( 1, &id );
    created= true;
}

void r_Texture::SetFiltration( GLuint min, GLuint mag )
{
    default_filtration= false;
    if (created )
    {
        BindTexture();
        if( mag_filtration != mag)
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag );
        if( min_filtration != min)
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min );
    }
    min_filtration= min;
    mag_filtration= mag;
}

void r_Texture::SetMaxLod( float n )
{
    if( created )
    {
        BindTexture();
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, (float)n );
    }
}

void r_Texture::SetMinLod( float n )
{
    if( created )
    {
        BindTexture();
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, (float)n );
    }
}

void r_Texture::SetDefaultFiltration()
{
    default_filtration= true;
    min_filtration= GL_LINEAR_MIPMAP_LINEAR;
    mag_filtration= GL_LINEAR;
}

void r_Texture::MoveOnGPU()
{
    if(created)
    {
        BindTexture();
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filtration );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filtration );

        //GL_EXT_texture_filter_anisotropic
        // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4 );

#ifdef OGL21//ogl 2.1 mipmaps autogeneration
        if( min_filtration == GL_LINEAR_MIPMAP_LINEAR || min_filtration == GL_LINEAR_MIPMAP_NEAREST
                || min_filtration == GL_NEAREST_MIPMAP_LINEAR || min_filtration == GL_NEAREST_MIPMAP_NEAREST )
            glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
#endif

        glTexImage2D( GL_TEXTURE_2D, 0, texture_type, width, height,
                      0, texture_type, data_type, texture_data);
#ifndef OGL21//ogl3 mipmap generation
        if( min_filtration == GL_LINEAR_MIPMAP_LINEAR || min_filtration == GL_LINEAR_MIPMAP_NEAREST
                || min_filtration == GL_NEAREST_MIPMAP_LINEAR || min_filtration == GL_NEAREST_MIPMAP_NEAREST )
            glGenerateMipmap( GL_TEXTURE_2D );
#endif

        in_video_memory= true;
    }
}

void r_Texture::TexSubData2GPU( unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char* data )
{
	if(created)
    {
        BindTexture();
        glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, w, h, texture_type, data_type, data );
    }
}

void r_Texture::DeleteFromGPU()
{
    glDeleteTextures( 1, &id );
    in_video_memory= false;
    created= false;
}

void r_Texture::DeleteFromRAM()
{
    if( texture_data != NULL )
        delete[] texture_data;

    texture_data= NULL;
    in_ram= false;
}

r_Texture::~r_Texture()
{
    DeleteFromGPU();
    DeleteFromRAM();
    if( current == this )
        current= NULL;
}
void r_Texture::TextureData( int w, int h, GLuint d_type, GLuint t_type, int bpp, void* data )
{
    if( texture_data != NULL )
        delete[] texture_data;

    width= w;
    height= h;
    data_type= d_type;
    texture_type= t_type;
    bits_per_pixel= bpp;
    texture_data= (unsigned char*) data;

    in_ram= true;
}

void rBGR2RGB( unsigned char* data, unsigned int size )
{
    unsigned int i;
    unsigned char tmp;
    for( i=0; i< size; i+=3 )
    {
        tmp= data[i];
        data[i]= data[i+2];
        data[i+2]= tmp;
    }
}

void rRGB_RChanenl2NormalMap(  unsigned char* src_data, unsigned char* dst_data,  unsigned int size_x, unsigned int size_y, float normal_z_size )
{
    unsigned int x, y;
    m_Vec3 v( 0.0f, 0.0f, 0.0f );
    m_Vec3 v2( 127.0f, 127.0f, 127.0f );


    unsigned int log2_x_m1, log2_y_m1;

    x= 1;
    while( x<= size_x )
        x<<=1;
    x>>=1;
    log2_x_m1= x-1;

    y= 1;
    while( y<= size_y )
        y<<=1;
    y>>=1;
    log2_y_m1= y-1;

    if( ( size_x & (size_x - 1 ) ) == 0 && ( size_y & (size_y - 1 ) ) == 0  )//if power of two texture
    {
        for( x= 0; x< size_x; x++ )
        {
            for( y= 0; y< size_y; y++ )
            {
                v.x= float( int( src_data[ ( y * size_x + ( (x-1) & log2_x_m1 ) ) * 3 ] ) - int( src_data[ ( y * size_x + ( (x+1) & log2_x_m1 ) ) * 3 ] ) );
                v.y= float( int( src_data[ ( ( (y-1) & log2_y_m1 ) * size_x + x ) * 3 ] ) - int( src_data[ ( ( (y+1) & log2_y_m1 ) * size_x + x ) * 3 ] ) );

                v*= 0.00392156f;// 1/255
                v.z= normal_z_size;
                v.Normalize();
                v*= 127.0f;
                v+= v2;
                dst_data[ ( y * size_x + x ) * 3     ]= (unsigned char)( v.x );
                dst_data[ ( y * size_x + x ) * 3 + 1 ]= (unsigned char)( v.y );
                dst_data[ ( y * size_x + x ) * 3 + 2 ]= (unsigned char)( v.z );
            }
        }
    }

    return;
}

void rBGRA2RGBA( unsigned char* data, unsigned int size )
{
    unsigned int i;
    unsigned char tmp;
    for( i=0; i< size; i+=4 )
    {
        tmp= data[i];
        data[i]= data[i+2];
        data[i+2]= tmp;
    }
}
void rRGB2RGBAKeyColor( r_TextureFile* tf, unsigned char key_color[3] )
{
    unsigned int i, j;
    unsigned char* new_data= new unsigned char[ tf->height * tf->width * 4 ];
    for( i= 0, j= 0; i< tf->data_size; i+=3, j+=4 )
    {
        new_data[j]= tf->data[i];
        new_data[j+1]= tf->data[i+1];
        new_data[j+2]= tf->data[i+2];
        if( tf->data[i] == key_color[0] &&
                tf->data[i+1] == key_color[1] &&
                tf->data[i+2] == key_color[2] )
            new_data[j+3]= 0;
        else
            new_data[j+3]= 255;
    }
    delete[] tf->data;
    tf->data= new_data;
    tf->data_size= tf->height * tf->width * 4 ;
    tf->texture_type= GL_RGBA;
    tf->bits_per_pixel= 32;
}

void rRGBAMirrorVertical( r_TextureFile* tf )
{
	for( int y= 0; y< tf->height>>1; y++ )
		for( int x= 0,
			k= y * tf->width,
			k2= ( tf->height - y - 1 ) * tf->width
			; x< tf->width; x++, k++, k2++ )
		{
			int* p = (int*) tf->data;
			int c= p[k];
			p[k]= p[k2];
			p[k2]= c;
		}
}

void rRGBAGetMip( r_TextureFile* tf_src, r_TextureFile* tf_dst )
{
	unsigned int x, y, i;
	unsigned int r,g,b,a;

	tf_dst->width= tf_src->width>>1;
	tf_dst->height= tf_src->height>>1;
	tf_dst->data_size= tf_dst->width * tf_dst->height * 4;
	tf_dst->texture_type= tf_src->texture_type;
	tf_dst->data_type= tf_src->data_type;

	for( y= 0; y< tf_dst->height; y++ )
	{
		for( x= 0; x< tf_dst->width; x++ )
		{
			i= ( x*2 + y*2 * tf_src->height ) * 4;
			r= (unsigned int)( tf_src->data[ i     ] );
			g= (unsigned int)( tf_src->data[ i + 1 ] );
			b= (unsigned int)( tf_src->data[ i + 2 ] );
			a= (unsigned int)( tf_src->data[ i + 3 ] );

			//i= ( x*2+1 + y*2 * tf_src->height ) * 4;
			i+=4;
			r+= (unsigned int)( tf_src->data[ i     ] );
			g+= (unsigned int)( tf_src->data[ i + 1 ] );
			b+= (unsigned int)( tf_src->data[ i + 2 ] );
			a+= (unsigned int)( tf_src->data[ i + 3 ] );

			//i= ( x*2+1 + (y*2+1) * tf_src->height ) * 4;
			i+= tf_src->height * 4;
			r+= (unsigned int)( tf_src->data[ i     ] );
			g+= (unsigned int)( tf_src->data[ i + 1 ] );
			b+= (unsigned int)( tf_src->data[ i + 2 ] );
			a+= (unsigned int)( tf_src->data[ i + 3 ] );

			//i= ( x*2 + (y*2+1) * tf_src->height ) * 4 ;
			i-= 4;
			r+= (unsigned int)( tf_src->data[ i     ] );
			g+= (unsigned int)( tf_src->data[ i + 1 ] );
			b+= (unsigned int)( tf_src->data[ i + 2 ] );
			a+= (unsigned int)( tf_src->data[ i + 3 ] );

			i= ( x + tf_dst->height * y ) * 4;
			tf_dst->data[ i     ]= (unsigned char)(r>>2);
			tf_dst->data[ i + 1 ]= (unsigned char)(g>>2);
			tf_dst->data[ i + 2 ]= (unsigned char)(b>>2);
			tf_dst->data[ i + 3 ]= (unsigned char)(a>>2);
		}
	}
}


int r_Texture::Load( const char* file )
{

    /*распознавание формата файла*/
    unsigned int len= strlen( file );
    r_TextureFile tf;
    if( !strcmp( file + len - 3, "bmp") || !strcmp( file + len - 3, "BMP") )
    {
        if ( rLoadTextureBMP( &tf, file ) )
            rDefaultTexture( &tf );
    }
    else if( !strcmp( file + len - 3, "jpg") || !strcmp( file + len - 3, "JPG") || !strcmp( file + len - 4, "JPEG") || !strcmp( file + len - 4, "jpeg") )
    {
        //if (rLoadTextureJPG( &tf, file ) )
        rDefaultTexture( &tf );
    }
    else if( !strcmp( file + len - 3, "tga") || !strcmp( file + len - 3, "TGA") )
    {
        if ( rLoadTextureTGA( &tf, file ) )
            rDefaultTexture( &tf );
    }
    else
        rDefaultTexture( &tf );
    /*распознавание формата файла*/

    Create();
    TextureData( tf.width, tf.height, tf.data_type, tf.texture_type, tf.bits_per_pixel, tf.data );
    MoveOnGPU();
    DeleteFromRAM();
    return 0;
}

#if 0
int rLoadTextureJPG( r_TextureFile* tf, const char* file_name, bool data_is_temp )
{
    jpeg_decompress_struct jpeg_file_struct;//информационная структура файла
    jpeg_error_mgr	       jpeg_error_struct;//структура ошибок
    JSAMPARRAY	       buffer;

    int row_stride;		///* physical row width in output buffer
    unsigned char  *bbuf;
    int jpeg_file_size;


    FILE* jpeg_file= fopen( file_name, "rb" );
    if( jpeg_file == NULL )
        return 1;

    fseek( jpeg_file, 0, SEEK_END );
    jpeg_file_size= ftell( jpeg_file );
    fseek( jpeg_file, 0, SEEK_SET );

    unsigned char* file_buffer= new unsigned char[ jpeg_file_size ];
    fread( file_buffer, 1, jpeg_file_size, jpeg_file );
    fclose( jpeg_file );

    jpeg_file_struct.err= jpeg_std_error(&jpeg_error_struct);

    jpeg_create_decompress( &jpeg_file_struct );
    jpeg_stdio_src( &jpeg_file_struct, file_buffer );
    jpeg_read_header( &jpeg_file_struct, TRUE );
    jpeg_start_decompress( &jpeg_file_struct );

    row_stride = jpeg_file_struct.output_width * jpeg_file_struct.output_components;
    tf->width= jpeg_file_struct.output_width;
    tf->height= jpeg_file_struct.output_height;

    tf->bits_per_pixel= 8 * jpeg_file_struct.output_components;
    if( tf->bits_per_pixel == 24 )
        tf->texture_type= GL_RGB;
    else if( tf->bits_per_pixel == 32 )
        tf->texture_type= GL_RGBA;
    else
        return 1;

    tf->data_size= tf->width * tf->height * jpeg_file_struct.output_components;
    tf->data= new unsigned char [ tf->data_size ];
    /*while (jpeg_file_struct.output_scanline < jpeg_file_struct.output_height  )
     {
    	bbuf = ( tf->data + ( row_stride * jpeg_file_struct.output_scanline) );
    	buffer = &bbuf;
    	jpeg_read_scanlines( &jpeg_file_struct, buffer, 1);
     }*/

    jpeg_file_struct.output_scanline= tf->height - 1;
    while (jpeg_file_struct.output_scanline + 1 > 0  )
    {
        bbuf = ( tf->data + ( row_stride * jpeg_file_struct.output_scanline) );
        buffer = &bbuf;
        jpeg_read_scanlines( &jpeg_file_struct, buffer, 1 );
        jpeg_file_struct.output_scanline-=2;
    }

    delete[] file_buffer;
    tf->data_type= GL_UNSIGNED_BYTE;
    return 0;
}
#endif
int rLoadTextureBMP( r_TextureFile* tf, const char* file_name, bool data_is_temp  )
{
    r_BitmapFileHeader fheader;
    r_BitmapInfoHeader iheader;

    FILE* file_bmp= fopen( file_name, "rb" );
    if( file_bmp == NULL )
        return 1;

    fread( &fheader, 1, sizeof(r_BitmapFileHeader), file_bmp );
    fread( &iheader, 1, sizeof(r_BitmapInfoHeader), file_bmp );

    if( sizeof(r_BitmapFileHeader) != 14 || sizeof(r_BitmapInfoHeader)!= 40 )
        printf( "invalid size of bmp structs\n" );

    if(iheader.biCompression!=BI_RGB || iheader.biHeight<0 ||(iheader.biBitCount!=24 && iheader.biBitCount!=32 &&  iheader.biBitCount!= 1) )
    {
        fclose( file_bmp );
        return 1;
    }

    tf->bits_per_pixel= iheader.biBitCount;
    tf->texture_type= ( tf->bits_per_pixel == 32 ) ? GL_RGBA : GL_RGB;
    tf->height= iheader.biHeight;
    tf->width= iheader.biWidth;

    tf->data_size= tf->width * tf->height *  tf->bits_per_pixel / 8;
    tf->data= new unsigned char[ tf-> data_size ];

    if( tf->data == NULL )
    {
        fclose( file_bmp );
        return 1;
    }


    fseek( file_bmp, fheader.bfOffBits, SEEK_SET );
    fread( tf->data, 1, tf->data_size, file_bmp );
    fclose( file_bmp );

    if( tf->bits_per_pixel == 1)
    {
        tf->data_size= tf->width * tf->height;
        unsigned char* tmp = new unsigned char[tf->data_size];
#if 1
        for( register unsigned int i= 0, j=0; i < tf->data_size; j++ )
        {
            tmp[i] = (tf->data[j] & 128);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 64);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 32);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 16);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 8);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 4);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 2);
            if(tmp[i]) tmp[i]=255;
            i++;
            tmp[i] = (tf->data[j] & 1);
            if(tmp[i]) tmp[i]=255;
            i++;

        }
#else
        int d_s= tf->data_size;
        void* tf_d= tf->data;
        __asm// по данному методу изображение 1920x1080 распаковывается менее чем за милисекунду
        {
            mov ebx, [tmp]// итоговые данные текстуры
            mov edx, [tf_d]// заппакованная текстура

            mov ecx, ebx
            add ecx, d_s// указатель на конец конечных данных

loop_p:

            mov ah, byte ptr [edx]// текущий байт исходных данных. содержит 8 пикселей

            mov al, ah
            and al, 128 ;
            10000000b
            jz p0
            mov al, 255
p0:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 64 ;
            1000000b
            jz p1
            mov al, 255
p1:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 32 ;
            100000b
            jz p2
            mov al, 255
p2:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 16 ;
            10000b
            jz p3
            mov al, 255
p3:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 8 ;
            1000b
            jz p4
            mov al, 255
p4:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 4 ;
            100b
            jz p5
            mov al, 255
p5:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 2 ;
            10b
            jz p6
            mov al, 255
p6:
            mov byte ptr[ebx], al
            inc ebx

            mov al, ah
            and al, 1 ;
            1b
            jz p7
            mov al, 255
p7:
            mov byte ptr[ebx], al
            inc ebx


            inc edx
            cmp ebx, ecx
            jnz loop_p

        }

#endif
        tf->texture_type= GL_RED;
        delete[] tf->data;
        tf->data= tmp;
        tf->bits_per_pixel= 8;
    }
    tf->data_type= GL_UNSIGNED_BYTE;
    if( tf->bits_per_pixel == 24 )
        rBGR2RGB( tf->data, tf->data_size );
    return 0;
}

int rLoadTextureTGA( r_TextureFile* tf, const char* file_name, bool data_is_temp  )
{
    unsigned char tga_header[8];
    char temp_buf[256];
    short int size_x, size_y, pos_x, pos_y;
    unsigned char bpp;
    //0- количество байт идентификатора
    //1- тип палитры (0 - её нет)
    //2- тип изображения. 2- true color 10 сжатое
    //3-7 описание палитры
    //8-9 координата по x
    //10-11 координата по y
    //12-13 ширина
    //14-15 высота
    //16 бит на пиксель
    //17 опистель изображения

    FILE* file_tga= fopen( file_name, "rb" );
    if ( file_tga == NULL )
        return 1;

    fread( tga_header, 8, 1, file_tga );
    if( tga_header[1] != 0 || tga_header[2] != 2 )// нужно изображение без палитры и без сжатия
    {
        fclose( file_tga );
        return 1;
    }

    fread( &pos_x, 1, 2, file_tga );
    fread( &pos_y, 1, 2, file_tga );
    fread( &size_x, 1, 2, file_tga );
    fread( &size_y, 1, 2, file_tga );

    if( size_x < 0 || size_x > 8192 || size_y < 0 || size_y > 8192 )// проверка на глючность
    {
        fclose( file_tga );
        return 1;
    }
    tf->width= size_x;
    tf->height= size_y;

    fread( &bpp, 1, 1, file_tga );
    tf->bits_per_pixel= bpp;
    if( tf->bits_per_pixel == 24 )
    {
        tf->texture_type= GL_RGB;
    }
    else if( tf->bits_per_pixel == 32 )
    {
        tf->texture_type= GL_RGBA;
    }
    else
    {
        fclose( file_tga );
        return 1;
    }
    // распознавание формата

    fread( temp_buf, 1, 1, file_tga );
    //file_tga.Read( temp_buf, 1 );// читаем описатель изображения, но он НЕ НУЖЕН
    if( tga_header[0] != 0 )
        fread( temp_buf, tga_header[0], 1, file_tga );

    tf->data_size= tf->height * tf->width * tf->bits_per_pixel /8;
    tf->data= new unsigned char[tf->data_size];
    if( tf->data == NULL )
    {
        fclose( file_tga );
        return 1;
    }

    fread( tf->data, tf->data_size, 1, file_tga );
    fclose( file_tga );
    tf->data_type= GL_UNSIGNED_BYTE;
    if( tf->bits_per_pixel == 24 )
        rBGR2RGB( tf->data, tf->data_size );
    else
        rBGRA2RGBA( tf->data, tf->data_size );
    return 0;
}


static unsigned char def_tex_data[48]= {  0,0,0,  0,0,0,  255,0,255,  255,0,255,
                                       /*дефолтная текстура -				    */0,0,0,  0,0,0,  255,0,255,  255,0,255,
                                       /*розово чёрные квадратики	    	    */255,0,255,  255,0,255,  0,0,0,  0,0,0,
                                       255,0,255,  255,0,255,  0,0,0,  0,0,0
                                       };
void rDefaultTexture( r_TextureFile* tf ,bool data_is_temp )
{

    tf->data= new unsigned char[ 48 ];
    if( tf->data != NULL )
        memcpy( tf->data, def_tex_data, 48 );

    tf->bits_per_pixel= 24;
    tf->height= 4;
    tf->width= 4;
    tf->data_size= 48;
    tf->texture_type= GL_RGB;
    tf->data_type= GL_UNSIGNED_BYTE;
}





int rSaveTextureTGA( const char* file_name, unsigned char* data, unsigned int sizeX, unsigned int sizeY )
{
    unsigned char  TGAheader[12]= {0,0,2,0,0,0,0,0,0,0,0,0};
    FILE* file_tga=fopen( file_name,"wb" );
    if (file_tga==NULL)
        return 1;
    unsigned char o_0=0;
    fwrite(TGAheader,1,12,file_tga);
    fwrite(&sizeX,1,2,file_tga);
    fwrite(&sizeY,1,2,file_tga);

    unsigned char bit=24;
    fwrite(&bit,1,1,file_tga);
    fwrite(&o_0,1,1,file_tga);
    int imagesize=sizeX*sizeY*3;
    unsigned char* data2=new unsigned char[imagesize];
    int step=bit/8;
    for (int i=0; i<imagesize; i+=step)
    {
        data2[i]=data[i+2];
        data2[i+1]=data[i+1];
        data2[i+2]=data[i];
    }
    fwrite(data2,1,imagesize,file_tga);
    fclose(file_tga);
    delete[] data2;

    return 0;
}





#endif//TEXTURE_CPP
