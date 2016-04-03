#include <cstring>

#include "../hex.hpp"

#include "ui_painter.hpp"
#include "matrix.hpp"
#include "../renderer/text.hpp"
#include "shaders_loading.hpp"

ui_Painter::ui_Painter()
{
	ui_vbo_.VertexData( nullptr, sizeof(ui_Vertex)*4096, sizeof(ui_Vertex) );

	ui_Vertex vert;
	int offset= ((char*)vert.coord) - ((char*)&vert);
	ui_vbo_.VertexAttribPointer( 0, 2, GL_SHORT, false, offset );
	offset= ((char*)vert.color) - ((char*)&vert);
	ui_vbo_.VertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, true, offset );

	r_GLSLVersion glsl_version( r_GLSLVersion::v330 );
	ui_shader_.ShaderSource( rLoadShader( "ui_frag.glsl", glsl_version ), rLoadShader( "ui_vert.glsl", glsl_version ) );

	ui_shader_.SetAttribLocation( "coord", 0 );
	ui_shader_.SetAttribLocation( "color", 1 );
	ui_shader_.Create();

	text_manager_.reset( new r_Text( "textures/mono_font_sdf.tga" ) );
}

ui_Painter::~ui_Painter()
{
}

void ui_Painter::SetMatrix( const m_Mat4& m )
{
	ui_shader_.Bind();
	ui_shader_.Uniform( "transform_matrix", m );
}

void ui_Painter::DrawUITriangles( ui_Vertex* vertices, int vertex_count, const unsigned char* color )
{
	ui_shader_.Bind();

	for( int i= 0; i< vertex_count; i++ )
	{
		vertices[i].color[0]= color[0];
		vertices[i].color[1]= color[1];
		vertices[i].color[2]= color[2];
		vertices[i].color[3]= color[3];
	}
	ui_vbo_.Bind();

	ui_vbo_.VertexSubData( vertices, vertex_count * sizeof(ui_Vertex), 0 );

	glDrawArrays( GL_TRIANGLES, 0, vertex_count );
}

void ui_Painter::DrawUIText( const char* text, float center_x, float center_y, float font_size, const unsigned char* font_color )
{
	float font_scaler= font_size / text_manager_->LetterHeight();

	float x= center_x - 0.5f * font_scaler * text_manager_->LetterWidth() * float( std::strlen(text) );
	float y= center_y - 0.5f * font_scaler * text_manager_->LetterHeight();

	text_manager_->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager_->Draw();
}

void ui_Painter::DrawUITextPixelCoordsLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color )
{
	text_manager_->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager_->Draw();
}

void ui_Painter::DrawUITextPixelCoordsCenter( const char* text, float center_x, float y, float font_size, const unsigned char* font_color )
{
	float x= center_x - 0.5f * ( float(strlen(text)) * font_size * float(text_manager_->LetterWidth()) / float(text_manager_->LetterHeight()) );
	text_manager_->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager_->Draw();
}

void ui_Painter::DrawUITextPixelCoordsRight( const char* text, float x, float y, float font_size, const unsigned char* font_color )
{
	x-= ( float(strlen(text)) * font_size * float(text_manager_->LetterWidth()) / float(text_manager_->LetterHeight()) );
	text_manager_->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager_->Draw();
}
