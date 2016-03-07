#pragma once

#include <vector>

#include "glsl_program.hpp"
#include "polygon_buffer.hpp"
#include "texture.hpp"

/*
 * Simple class for drawing simple texts.
 * Can draw 96 ASCII symbols with codes [32;127).
 * Supported fonts - only monospace.
 * Input font - texture, where letters placed top to down, like this:
 *  ___
 * | A |
 * | B |
 * | C |
 * | D |
 * |___|
 *
 * Drawing is buffered. You need call AddText, and then, call Draw.
 * Inner buffer size is large enough for big texts.
 */

class r_Text
{
public:
	r_Text( const char* font_file );
	~r_Text();

	void AddText( float column, float row, float size/*in letters*/, const unsigned char* color, const char* text );
	void AddMultiText( float column, float row, float size /*in letters*/, const unsigned char* color, const char* text, ... );

	void AddTextPixelCoords( float x, float y, float size/*in pixels*/, const unsigned char* color, const char* text );
	void Draw();

	float LetterWidth () const;
	float LetterHeight() const;

public:
	static const unsigned char default_color[4];

private:
	#pragma pack( push, 1 )
	struct r_TextVertex
	{
		float pos[2];
		unsigned char color[4];
		unsigned char tex_coord[2];
		unsigned char reserved[2];
	};
	#pragma pack (pop)
	static_assert(
		sizeof(r_TextVertex) == 16,
		"Unexpected size. It will be perfect, if size of this structure will be 16 bytes.");

private:
	r_GLSLProgram shader_;
	r_PolygonBuffer text_vbo_;
	r_Texture font_texture_;

	std::vector<r_TextVertex> vertices_;
	unsigned int vertex_buffer_pos_;

	unsigned int letter_width_, letter_height_;

private:
	static const constexpr unsigned int c_letters_in_texture_= 96;
	static const constexpr unsigned int c_text_buffer_size_= 8192;
};

inline float r_Text::LetterWidth () const
{
	return float(letter_width_);
}

inline float r_Text::LetterHeight() const
{
	return float(letter_height_);
}
