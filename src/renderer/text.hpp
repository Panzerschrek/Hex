#pragma once

#include "glsl_program.hpp"
#include "polygon_buffer.hpp"
#include "framebuffer_texture.hpp"

class Texture;

#pragma pack( push, 1 )
struct r_TextVertex
{
	float pos[2];
	unsigned char color[4];
	unsigned char tex_coord[2];
	unsigned char texles_per_pixel[2]; // in 4.4 format
};
#pragma pack (pop)


#define H_MAX_TEXT_BUFFER_SIZE 8192

#define LETTERS_IN_TEXTURE 96

class r_Text
{
public:

	r_Text( const char* font_file );
	~r_Text() {}
	void AddText( float colomn, float row, float, const unsigned char* color, const char* text );
	void AddMultiText( float colomn, float row, float, const unsigned char* color, const char* text, ... );

	void AddTextPixelCoords( float x, float y, float size/*in pixels*/, const unsigned char* color, const char* text );
	void Draw();
	void SetViewport( unsigned int x, unsigned int y );

	unsigned int ColomnsInScreen()const;
	unsigned int RowsInScreen()const;

	float LetterWidth() const;
	float LetterHeight() const;

public:
	static const unsigned char default_color[4];

private:

	r_GLSLProgram text_shader;
	r_TextVertex* vertices;
	r_PolygonBuffer text_vbo;
	unsigned int vertex_buffer_size;
	unsigned int vertex_buffer_pos;
	float screen_x, screen_y;

	unsigned char* text_texture_data;
	unsigned int letter_width, letter_height;

	r_FramebufferTexture font_texture;
};

inline void r_Text::SetViewport( unsigned int x, unsigned int y )
{
	screen_x= float(x);
	screen_y= float(y);
}

inline unsigned int r_Text::ColomnsInScreen()const
{
	return screen_x / letter_width;
}

inline unsigned int r_Text::RowsInScreen()const
{
	return screen_y / letter_height;
}

inline float r_Text::LetterWidth() const
{
	return float(letter_width);
}

inline float r_Text::LetterHeight() const
{
	return float(letter_height);
}
