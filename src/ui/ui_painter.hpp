#ifndef UI_PAINTER_HPP
#define UI_PAINTER_HPP

#include "../renderer/polygon_buffer.h"
#include "../renderer/glsl_program.h"

class m_Mat4;
class r_Text;


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
	void DrawUITriangles( ui_Vertex* vertices, int vertex_count, const unsigned char* color );
	void DrawUIText( const char* text, float center_x, float center_y, float font_size, const unsigned char* font_color );

	//coordinates - upper left corner of first letter
	void DrawUiTextLeft( const char* text, float x, float y, float font_size, const unsigned char* font_color );

	private:

	r_GLSLProgram ui_shader;
	r_PolygonBuffer ui_vbo;
	r_Text* text_manager;
};



#endif//UI_PAINTER_HPP
