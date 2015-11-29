#pragma once
#include "../fwd.hpp"
#include "polygon_buffer.hpp"
#include "glsl_program.hpp"

class m_Mat4;

#pragma pack( push, 1 )
struct ui_Vertex
{
	short coord[2];
	unsigned char color[4];
};
#pragma pack(pop)

class ui_Painter
{
public:
	ui_Painter();
	~ui_Painter();

	void SetMatrix( const m_Mat4& m );
	//input - vertices in xy format
	void DrawUITriangles(ui_Vertex* vertices, int vertex_count, const unsigned char* color );
	void DrawUIText( const char* text, float center_x, float center_y, float font_size, const unsigned char* font_color );

	//coordinates - upper left corner of first letter
	void DrawUITextLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color );

	void DrawUITextPixelCoordsLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color );
	void DrawUITextPixelCoordsCenter( const char* text, float center_x, float y, float font_size, const unsigned char* font_color );
	void DrawUITextPixelCoordsRight( const char* text, float x, float y, float font_size, const unsigned char* font_color );

private:
	r_GLSLProgram ui_shader;
	r_PolygonBuffer ui_vbo;
	r_Text* text_manager;
};
