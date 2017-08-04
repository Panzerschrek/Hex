#include <cstdio>

#include "texture_manager.hpp"
#include "img_utils.hpp"
#include "rendering_constants.hpp"
#include "../block.hpp"
#include "../console.hpp"

#include <PanzerJson/parser.hpp>
#include <PanzerJson/value.hpp>

#include <QImage>
#include <QPainter>

unsigned char r_TextureManager::texture_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];
bool r_TextureManager::texture_mode_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];
unsigned char r_TextureManager::texture_scale_table_[ size_t(h_BlockType::NumBlockTypes) * 8 ];

static const GLuint c_texture_not_created= std::numeric_limits<GLuint>::max();

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
{return;
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

r_TextureManager::r_TextureManager( unsigned int detalization, bool filter_textures )
	: texture_array_( c_texture_not_created )
	, texture_detalization_( std::min( detalization, (unsigned int)(R_MAX_TEXTURE_RESOLUTION_LOG2 - R_MIN_TEXTURE_RESOLUTION_LOG2) ) )
	, filter_textures_( filter_textures )
{
	InitTextureTable();
	LoadWorldTextures();

	r_ImgUtils::LoadTexture( &water_texture_, "textures/water2.tga", texture_detalization_ );
	if( filter_textures_ )
		water_texture_.SetFiltration( r_Texture::Filtration::LinearMipmapLinear, r_Texture::Filtration::Linear );
	else
		water_texture_.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
}

r_TextureManager::~r_TextureManager()
{
	glDeleteTextures( 1, &texture_array_ );
}

void r_TextureManager::BindTextureArray( unsigned int unit )
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_2D_ARRAY, texture_array_ );
}

void r_TextureManager::BindWaterTexture( unsigned int unit )
{
	water_texture_.Bind( unit );
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

void r_TextureManager::LoadWorldTextures()
{
	unsigned int texture_size= 1 << (R_MAX_TEXTURE_RESOLUTION_LOG2 - texture_detalization_);

	unsigned int tex_id= 0;
	QSize required_texture_size( R_MAX_TEXTURE_RESOLUTION, R_MAX_TEXTURE_RESOLUTION );
	std::vector<unsigned char> tmp_data( R_MAX_TEXTURE_RESOLUTION * R_MAX_TEXTURE_RESOLUTION * 4 );

	PanzerJson::Parser parser;
	PanzerJson::Value json_root_array;
	PanzerJson::Parser::ResultPtr parse_result;
	{
		const char* const config_file_name= "textures/textures.json";
		std::FILE* const f= std::fopen( config_file_name, "rb" );
		if( f != nullptr )
		{
			std::fseek( f, 0, SEEK_END );
			const size_t file_size= std::ftell( f );
			std::fseek( f, 0, SEEK_SET );

			std::vector<char> file_content( file_size );
			std::fread( file_content.data(), 1, file_size, f ); // TODO - check file errors
			std::fclose(f);

			parse_result= parser.Parse( file_content.data(), file_content.size() );
			if( parse_result->error != PanzerJson::Parser::Result::Error::NoError )
				h_Console::Error( "Error, parsing json config" );
			json_root_array= parse_result->root;
		}
		else
			h_Console::Error( "fatal error, textures config file \"", config_file_name, "\" not found" );
	}
	unsigned texture_layers= std::min( int(json_root_array.ElementCount()) + 1, R_MAX_TEXTURES );

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
	for( const PanzerJson::Value obj : json_root_array.array_elements() )
	{
		if( ! obj.IsObject() )
			continue;

		const char* const filename= obj[ "filename" ].AsString();
		if( !img.load( QString("textures/") + filename ) )
		{
			h_Console::Warning( "texture \"", filename, "\" not found" );
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
				filename,
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

		const PanzerJson::Value scale_val= obj[ "scale" ];
		if( scale_val.IsNumber() )
			texture_scale_table_[ tex_id ]= std::min( std::max( scale_val.AsInt(), 1 ), H_TEXTURE_SCALE_MULTIPLIER * H_TEXTURE_MAX_SCALE );

		const PanzerJson::Value perblock_val= obj[ "perblock" ];
		if( perblock_val.IsBool() )
			texture_mode_table_[ tex_id ]= perblock_val.AsInt() != 0;

		const PanzerJson::Value blocks= obj[ "blocks" ];

		//for each block for texture
		for( const PanzerJson::Value block_obj : blocks )
		{
			const char* const blockname= block_obj[ "blockname" ].AsString();
			const char* const blockside= block_obj[ "blockside" ].AsString();

			h_BlockType t= h_Block::GetGetBlockTypeByName( blockname );
			if( t == h_BlockType::Unknown ) continue;

			if( std::strcmp( blockside, "universal" ) == 0 )
				for( unsigned int k= 0; k< 8; k++ )
					texture_table_[ ( static_cast<unsigned int>(t) << 3 ) | k ]= tex_id;

			h_Direction d= h_Block::GetDirectionByName( blockside );
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
