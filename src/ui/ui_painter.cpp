#include "../hex.hpp"

#include "ui_painter.hpp"
#include "matrix.hpp"
#include "../renderer/text.hpp"

ui_Painter::ui_Painter()
{
	ui_vbo.VertexData( nullptr, sizeof(ui_Vertex)*4096, sizeof(ui_Vertex) );

	ui_Vertex vert;
	int offset= ((char*)vert.coord) - ((char*)&vert);
	ui_vbo.VertexAttribPointer( 0, 2, GL_SHORT, false, offset );
	offset= ((char*)vert.color) - ((char*)&vert);
	ui_vbo.VertexAttribPointer( 1, 4, GL_UNSIGNED_BYTE, true, offset );

	ui_shader.Load( "shaders/ui_frag.glsl", "shaders/ui_vert.glsl" );
	ui_shader.SetAttribLocation( "coord", 0 );
	ui_shader.SetAttribLocation( "color", 1 );
	ui_shader.Create();

	//static const char*const font_files[]= { "textures/courier_new_18.bmp", "textures/courier_new_24.bmp", "textures/courier_new_32.bmp" };
	//text_manager= new r_Text( font_files[1] );
	text_manager= new r_Text( "textures/mono_font_sdf.tga" );
}

ui_Painter::~ui_Painter()
{
}

void ui_Painter::SetMatrix( const m_Mat4& m )
{
	ui_shader.Bind();

	ui_shader.Uniform( "transform_matrix", m );
}

void ui_Painter::DrawUITriangles( ui_Vertex* vertices, int vertex_count, const unsigned char* color )
{
	ui_shader.Bind();

	for( int i= 0; i< vertex_count; i++ )
	{
		vertices[i].color[0]= color[0];
		vertices[i].color[1]= color[1];
		vertices[i].color[2]= color[2];
		vertices[i].color[3]= color[3];
	}
	ui_vbo.Bind();

	ui_vbo.VertexSubData( vertices, vertex_count * sizeof(ui_Vertex), 0 );

	glDrawArrays( GL_TRIANGLES, 0, vertex_count );
}

void ui_Painter::DrawUIText( const char* text, float center_x, float center_y, float font_size, const unsigned char* font_color )
{
	int len= strlen(text);
	float x= center_x - text_manager->LetterWidth() * 0.5f * float(len);
	float y= center_y - 0.5f * text_manager->LetterHeight();
	text_manager->AddText( x / text_manager->LetterWidth(), y/text_manager->LetterHeight(), font_size, font_color, text );
	text_manager->Draw();
}

void ui_Painter::DrawUITextLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color )
{
	text_manager->AddText( x / text_manager->LetterWidth(), y/text_manager->LetterHeight(), font_size, font_color, text );
	text_manager->Draw();
}

void ui_Painter::DrawUITextPixelCoordsLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color )
{
	text_manager->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager->Draw();
}

void ui_Painter::DrawUITextPixelCoordsCenter( const char* text, float center_x, float y, float font_size, const unsigned char* font_color )
{
	float x= center_x - 0.5f * ( float(strlen(text)) * font_size * float(text_manager->LetterWidth()) / float(text_manager->LetterHeight()) );
	text_manager->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager->Draw();
}

void ui_Painter::DrawUITextPixelCoordsRight( const char* text, float x, float y, float font_size, const unsigned char* font_color )
{
	x-= ( float(strlen(text)) * font_size * float(text_manager->LetterWidth()) / float(text_manager->LetterHeight()) );
	text_manager->AddTextPixelCoords( x, y, font_size, font_color, text );
	text_manager->Draw();
}
