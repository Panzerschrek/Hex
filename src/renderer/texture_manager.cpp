#ifndef TEXTURE_MANAGER_CPP
#define TEXTURE_MANAGER_CPP
#include "texture_manager.hpp"
#include "img_utils.hpp"
#include "rendering_constants.hpp"
#include "../block.hpp"
#include "../console.hpp"
#include <iostream>
#include <fstream>


#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>



unsigned char r_TextureManager::texture_table[ NUM_BLOCK_TYPES * 8 ];
bool r_TextureManager::texture_mode_table[ NUM_BLOCK_TYPES * 8 ];
unsigned char r_TextureManager::texture_scale_table[ NUM_BLOCK_TYPES * 8 ];

void r_TextureManager::InitTextureTable()
{
    unsigned int i;
    for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
        texture_table[i]= 0;
    for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
        texture_mode_table[i]= false;
    for( i= 0; i< NUM_BLOCK_TYPES * 8; i++ )
        texture_scale_table[i]= H_MAX_TEXTURE_SCALE;
}

void r_TextureManager::DrawNullTexture( QImage* img, const char* text )
{
    QPainter p( img );

    p.fillRect( 0, 0, 128, 128, Qt::black );
    p.fillRect( 128, 0, 128, 128, Qt::magenta );
    p.fillRect( 0, 128, 128, 128, Qt::magenta );
    p.fillRect( 128, 128, 128, 128, Qt::black );

    p.fillRect( 112, 112, 16, 16, Qt::red );
    p.fillRect( 128, 112, 16, 16, Qt::green );
    p.fillRect( 112, 128, 16, 16, Qt::blue );
    p.fillRect( 128, 128, 16, 16, Qt::gray );

    QFont f( "Courier New", 16 );
    p.setFont( f );
    p.setPen( QColor( Qt::white ) );
    p.drawText( 20, 80, "Texture not found" );
    p.drawText( 20, 130, text );
    p.drawText( 20, 180, "Texture not found" );
}

r_TextureManager::r_TextureManager()
{
    InitTextureTable();
}

void r_TextureManager::LoadTextures()
{
	unsigned int texture_layers= 32;
    unsigned int tex_id= 0;
    char str[256];


 	glGenTextures( 1, &texture_array );
	glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array );
    glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, texture_size, texture_size, texture_layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    QImage img( QSize( 256, 256 ), QImage::Format_RGBA8888 );

    //"load" null texture
    DrawNullTexture( &img );

    unsigned char* tmp_data= new unsigned char[ R_MAX_TEXTURE_RESOLUTION * R_MAX_TEXTURE_RESOLUTION * 4 ];
    unsigned char* tex_data_pointers[2];
    tex_data_pointers[0]= (unsigned char*) img.constBits();
    tex_data_pointers[1]= tmp_data;
    int s= 0, d= 1;
    for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
    	r_ImgUtils::RGBA8_GetMip( tex_data_pointers[s], tex_data_pointers[d], i, i );
	r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( tex_data_pointers[s], texture_size, texture_size );
	glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, tex_id, texture_size, texture_size,
                     1, GL_RGBA, GL_UNSIGNED_BYTE, tex_data_pointers[s] );


    const char* config_file_name=  "textures/textures.json";
    QString fn( config_file_name );
    QFile f( fn );
    if( !f.open( QIODevice::ReadOnly ) )
        h_Console::Error( "fatal error, file \"%s\" not found", config_file_name );
    QByteArray ba= f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc= QJsonDocument::fromJson( ba, &err );

    if( doc.isArray() )
    {
        QJsonArray arr= doc.array();

        //for each texture
        for( int i= 0; i< arr.size(); i++ )
        {
            QJsonObject obj= arr.at(i).toObject();
            QJsonValue val;
            val= obj[ "filename" ];

            if( ! img.load( obj[ "filename" ].toString() ) )
            {
               	h_Console::Warning( "texture \"%s\" not fund", obj[ "filename" ].toString().toLocal8Bit().constData() );
                continue;
            }
            else
            {
            	tex_id++;

                tex_data_pointers[0]= (unsigned char*) img.constBits();
				tex_data_pointers[1]= tmp_data;
				s= 0, d= 1;
				for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
					r_ImgUtils::RGBA8_GetMip( tex_data_pointers[s], tex_data_pointers[d], i, i );
				r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( tex_data_pointers[s], texture_size, texture_size );
				glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0, 0, 0, tex_id, texture_size, texture_size,
								 1, GL_RGBA, GL_UNSIGNED_BYTE, tex_data_pointers[s] );
            }

            val= obj[ "scale" ];
            if( val.isDouble() )
                texture_scale_table[ tex_id ]= min( max( val.toInt(), 1 ), H_MAX_TEXTURE_SCALE * H_MAX_TEXTURE_SCALE );

            val= obj[ "perblock" ];
            if( val.isBool() )
                texture_mode_table[ tex_id ]= val.toBool();

            QJsonArray blocks= obj[ "blocks" ].toArray();

            //for each block for texture
            for( int j= 0; j< blocks.size(); j++ )
            {
                QJsonObject block_obj= blocks.at(j).toObject();

                QString blockname, blockside;

                val= block_obj[ "blockname" ];
                blockname= val.toString();

                val= block_obj[ "blockside" ];
                blockside= val.toString();


                h_BlockType t= h_Block::GetGetBlockTypeByName( blockname.toLocal8Bit().data() );

                if( ! strcmp( blockside.toLocal8Bit().data(), "universal" )  )
                    for( int k= 0; k< 8; k++ )
                        texture_table[ (t<<3) | k ]= tex_id;

                h_Direction d= h_Block::GetDirectionByName( blockside.toLocal8Bit().data() );
                if( d != DIRECTION_UNKNOWN && t!= BLOCK_UNKNOWN  )
                    texture_table[ (t<<3) |d ]= tex_id;

            }//for blocks

        }//for textures
    }//if json array
    else
        h_Console::Error( "invalid JSON file: \"%s\"\n", config_file_name );

	delete[] tmp_data;

	if( filter_textures )
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	}
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

#endif//TEXTURE_MANAGER_CPP
