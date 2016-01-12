#include "texture_manager.hpp"
#include "img_utils.hpp"
#include "rendering_constants.hpp"
#include "../block.hpp"
#include "../console.hpp"

#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QPainter>

unsigned char r_TextureManager::texture_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];
bool r_TextureManager::texture_mode_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];
unsigned char r_TextureManager::texture_scale_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];

static const GLuint c_texture_not_created= std::numeric_limits<GLuint>::max();

static void ValidateTexturesConfig( const QJsonDocument& doc )
{
	h_Console::Info( "Validate textures config." );

	if( !doc.isArray() )
	{
		h_Console::Error( "Expected, that textures config is valid json array" );
		return;
	}

	const QJsonArray array= doc.array();
	for( const QJsonValue value : array )
	{
		if( !value.isObject() )
		{
			h_Console::Warning( "Expected, that values in array are objects" );
			continue;
		}

		const QJsonObject object= value.toObject();

		if( !object.contains( "filename" ) )
			h_Console::Warning( "Expected, that objects contains property \"filename\"" );

		const QJsonValue blocks= object["blocks"];
		if( blocks == QJsonValue::Undefined )
		{
			h_Console::Warning( "Expected, that objects contains \"blocks\" array" );
			continue;
		}

		if( !blocks.isArray() )
		{
			h_Console::Warning( "Expected, that \"blocks\" is array" );
			continue;
		}

		const QJsonArray blocks_array= blocks.toArray();
		for( const QJsonValue block_value : blocks_array )
		{
			if( !block_value.isObject() )
			{
				h_Console::Warning( "Expected, that array \"blocks\" contains objects" );
				continue;
			}
			const QJsonObject block_object= block_value.toObject();

			if( !block_object.contains( "blockname" ) )
				h_Console::Warning( "Expected, that \"block\" object has property \"\blockname\"" );

			if( !block_object.contains( "blockside" ) )
				h_Console::Warning( "Expected, that \"block\" object has property \"\blockside\"" );

		} // for blocks

		if( blocks_array.size() == 0 )
		{
			h_Console::Warning( "Expected, that \"blocks\" is non empty array" );
			continue;
		}
	} // for objects in array
}

static unsigned char* RescaleAndPrepareTexture( unsigned char* in_data, unsigned char* tmp_data, unsigned int requrided_size )
{
	unsigned int s= 0, d= 1;
	unsigned char* tex_data_pointers[2];
	tex_data_pointers[0]= in_data;
	tex_data_pointers[1]= tmp_data;

	for( unsigned int i= R_MAX_TEXTURE_RESOLUTION; i > requrided_size; i>>=1, s^=1, d^=1 )
		r_ImgUtils::RGBA8_GetMip( tex_data_pointers[s], tex_data_pointers[d], i, i );

	r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( tex_data_pointers[s], requrided_size, requrided_size );
	return tex_data_pointers[s];
}

static void DrawNullTexture( QImage& img, const char* text= nullptr )
{
	QPainter p( &img );

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
	if( text) p.drawText( 20, 130, text );
	p.drawText( 20, 180, "Texture not found" );
}

r_TextureManager::r_TextureManager()
	: texture_array_( c_texture_not_created )
	, texture_detalization_( 0 )
	, filter_textures_( true )
{
	InitTextureTable();
}

r_TextureManager::~r_TextureManager()
{
	if( texture_array_ != c_texture_not_created )
	{
		glDeleteTextures( 1, &texture_array_ );
	}
}

void r_TextureManager::InitTextureTable()
{
	for( unsigned char& tex : texture_table_ )
		tex= 0;
	for( bool& tex_mode : texture_mode_table_ )
		tex_mode= false;
	for( unsigned char& tex_scale : texture_scale_table_ )
		tex_scale= H_TEXTURE_SCALE_MULTIPLIER;
}

void r_TextureManager::LoadTextures()
{
	unsigned int texture_size= 1 << (R_MAX_TEXTURE_RESOLUTION_LOG2 - texture_detalization_);

	unsigned int tex_id= 0;
	QSize required_texture_size( R_MAX_TEXTURE_RESOLUTION, R_MAX_TEXTURE_RESOLUTION );
	std::vector<unsigned char> tmp_data( R_MAX_TEXTURE_RESOLUTION * R_MAX_TEXTURE_RESOLUTION * 4 );

	QJsonArray arr;
	{
		const char* config_file_name= "textures/textures.json";
		QFile f( config_file_name );
		if( f.open( QIODevice::ReadOnly ) )
		{
			QByteArray ba= f.readAll();
			f.close();

			QJsonDocument doc= QJsonDocument::fromJson( ba );
			ValidateTexturesConfig( doc );
			arr= doc.array();
		}
		else
			h_Console::Error( "fatal error, textures config file \"", config_file_name, "\" not found" );

	}
	unsigned texture_layers= std::min( arr.size() + 1, R_MAX_TEXTURES );

	glGenTextures( 1, &texture_array_ );
	glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		texture_size, texture_size, texture_layers,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	QImage img( required_texture_size, QImage::Format_RGBA8888 );

	DrawNullTexture( img );
	glTexSubImage3D(
		GL_TEXTURE_2D_ARRAY,
		0, 0, 0,
		tex_id,
		texture_size, texture_size,
		1, GL_RGBA, GL_UNSIGNED_BYTE,
		RescaleAndPrepareTexture( img.bits(), tmp_data.data(), texture_size ) );

	//for each texture
	for( const QJsonValue arr_val : arr )
	{
		QJsonObject obj= arr_val.toObject();
		QJsonValue val;

		QString filename= obj[ "filename" ].toString();
		if( !img.load( "textures/" + filename ) )
		{
			h_Console::Warning( "texture \"", filename.toLocal8Bit().constData(), "\" not found" );
			continue;
		}

		tex_id++;
		if( tex_id == R_MAX_TEXTURES )
		{
			h_Console::Warning( "Too much textures! ", R_MAX_TEXTURES, " is maximum" );
			break;
		}

		if( img.size() != required_texture_size )
		{
			h_Console::Warning(
				"texture \"",
				filename.toLocal8Bit().constData(),
				"\" has unexpected size: ", img.width(), "x", img.height() ," expected ",
				required_texture_size.width(), "x", required_texture_size.height() );

			img= img.scaled( required_texture_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY,
			0, 0, 0,
			tex_id,
			texture_size, texture_size,
			1, GL_RGBA, GL_UNSIGNED_BYTE,
			RescaleAndPrepareTexture( img.bits(), tmp_data.data(), texture_size ) );

		val= obj[ "scale" ];
		if( val.isDouble() )
			texture_scale_table_[ tex_id ]= std::min( std::max( val.toInt(), 1 ), H_TEXTURE_SCALE_MULTIPLIER * H_TEXTURE_MAX_SCALE );

		val= obj[ "perblock" ];
		if( val.isBool() )
			texture_mode_table_[ tex_id ]= val.toBool();

		QJsonArray blocks= obj[ "blocks" ].toArray();

		//for each block for texture
		for( const QJsonValue block_val : blocks )
		{
			QJsonObject block_obj= block_val.toObject();

			QString blockname= block_obj[ "blockname" ].toString();
			QString blockside= block_obj[ "blockside" ].toString();

			h_BlockType t= h_Block::GetGetBlockTypeByName( blockname.toLocal8Bit().data() );
			if( t == h_BlockType::Unknown ) continue;

			if( blockside == "universal" )
				for( unsigned int k= 0; k< 8; k++ )
					texture_table_[ ( static_cast<unsigned int>(t) << 3 ) | k ]= tex_id;

			h_Direction d= h_Block::GetDirectionByName( blockside.toLocal8Bit().data() );
			if( d != h_Direction::Unknown )
				texture_table_[ ( static_cast<unsigned int>(t) << 3 ) | static_cast<unsigned int>(d) ]= tex_id;
		}//for blocks
	}//for textures

	if( filter_textures_ )
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

void r_TextureManager::BindTextureArray( unsigned int unit )
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array_ );
}

void r_TextureManager::SetTextureDetalization( unsigned int detalization )
{
	texture_detalization_= std::min( detalization, (unsigned int)(R_MAX_TEXTURE_RESOLUTION_LOG2 - R_MIN_TEXTURE_RESOLUTION_LOG2) );
}

void r_TextureManager::SetFiltration( bool filter_textures )
{
	filter_textures_= filter_textures;
}

