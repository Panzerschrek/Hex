#ifndef TEXTURE_MANAGER_CPP
#define TEXTURE_MANAGER_CPP
#include "texture_manager.hpp"
#include "rendering_constants.hpp"
#include "../block.hpp"
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

void r_TextureManager::DrawNullTexture( QImage* img )
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
    p.drawText( 20, 180, "Texture not found" );
}

r_TextureManager::r_TextureManager()
{
    InitTextureTable();
}

void r_TextureManager::LoadTextures()
{
    unsigned int tex_id= 0;
    r_TextureFile tf[2];
    char str[256];

    tf[1].data= new unsigned char[ R_MAX_TEXTURE_RESOLUTION * R_MAX_TEXTURE_RESOLUTION * 4 ];

    texture_array.TextureData( texture_size, texture_size, 32, GL_UNSIGNED_BYTE, GL_RGBA, 32, NULL );
    texture_array.MoveOnGPU();

    QImage img( QSize( 256, 256 ), QImage::Format_RGBA8888 );

	//"load" null texture
	DrawNullTexture( &img );
    tf[0].data= (unsigned char*)img.constBits();//using inner QImage memory to increase perfomanse
    tf[0].width= img.width();
    tf[0].height= img.height();
    tf[0].data_size= tf[0].width * tf[0].height * 4;

    int s= 0, d=1;
    for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
        rRGBAGetMip( &tf[s], &tf[d] );

	rRGBAMirrorVerticalAndSwapRB( &tf[s] );//convert image from inner QImage format to OpenGL RGBA8 format
    texture_array.TextureLayer( tex_id, tf[s].data );

    const char* config_file_name=  "textures/textures.json";
    QString fn( config_file_name );
    QFile f( fn );
    if( !f.open( QIODevice::ReadOnly ) )
    {
        printf( "fatal error, file \"%s\" not found\n", config_file_name );
    }
    QByteArray ba= f.readAll();

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
                printf( "error, texture \"%s\" not fund\n", obj[ "filename" ].toString().toLocal8Bit().data() );
            else
            {
                tf[0].data= (unsigned char*)img.constBits();//using inner QImage memory to increase perfomanse
                tf[0].width= img.width();
                tf[0].height= img.height();
                tf[0].data_size= tf[0].width * tf[0].height * 4;
                tex_id++;
                int s= 0, d=1;
                for( int i= R_MAX_TEXTURE_RESOLUTION; i > texture_size; i>>=1, s^=1, d^=1 )
                    rRGBAGetMip( &tf[s], &tf[d] );

                rRGBAMirrorVerticalAndSwapRB( &tf[s] );//convert image from inner QImage format to OpenGL RGBA8 format
                texture_array.TextureLayer( tex_id, tf[s].data );
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
    {
        printf( "invalid JSON file: \"%s\"\n", config_file_name );
    }

    delete[] tf[1].data;
    if( filter_textures )
        texture_array.SetFiltration( GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR );
    else
        texture_array.SetFiltration( GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST );
    texture_array.GenerateMipmap();
}

#endif//TEXTURE_MANAGER_CPP
