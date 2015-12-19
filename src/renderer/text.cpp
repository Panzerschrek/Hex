#include "ogl_state_manager.hpp"

#include "../console.hpp"
#include "img_utils.hpp"

#include "text.hpp"

const unsigned char r_Text::default_color[4]= {255, 255, 255, 255 };
const constexpr unsigned int r_Text::c_letters_in_texture_;
const constexpr unsigned int r_Text::c_text_buffer_size_;

static inline void ColorCopy( unsigned char* dst, unsigned const char* src )
{
	*((int*)dst)= *((const int*)src);
}

void r_Text::AddMultiText( float colomn, float row, float size, const unsigned char* color, const char* text, ... )
{
	static char str[ c_text_buffer_size_ ];
	va_list ap;
	va_start( ap, text );
	vsprintf( str, text, ap );
	va_end( ap );

	AddText( colomn, row, size, color, str );
}

void r_Text::AddText( float colomn, float row, float size, const unsigned char* color, const char* text )
{
	const char* str= text;

	float x, x0, y;
	float dx, dy;

	float d_size= 2.0f * size;

	x0= x= d_size * colomn * float( letter_width_ ) / viewport_width_ - 1.0f;
	y=  -d_size * (row + 1) * float( letter_height_ ) / viewport_height_ + 1.0f;

	dx= (d_size * float(letter_width_ ) ) / viewport_width_;
	dy= (d_size * float(letter_height_) ) / viewport_height_;

	unsigned char texels_per_pixel= (unsigned char)( 16.0f / size );

	r_TextVertex* v= vertices_.data() + vertex_buffer_pos_;
	while( *str != 0 )
	{

		if( *str == '\n' )
		{
			x= x0;
			y-=dy;
			str++;
			continue;
		}

		int symb_pos= 95 - (*str-32);
		v[0].pos[0]= x;
		v[0].pos[1]= y;
		v[0].tex_coord[0]= 0;
		v[0].tex_coord[1]= symb_pos;

		v[1].pos[0]= x;
		v[1].pos[1]= y + dy;
		v[1].tex_coord[0]= 0;
		v[1].tex_coord[1]= symb_pos + 1;

		v[2].pos[0]= x + dx;
		v[2].pos[1]= y + dy;
		v[2].tex_coord[0]= 1;
		v[2].tex_coord[1]= symb_pos + 1;

		v[3].pos[0]= x + dx;
		v[3].pos[1]= y;
		v[3].tex_coord[0]= 1;
		v[3].tex_coord[1]= symb_pos;

		ColorCopy( v[0].color, color );
		ColorCopy( v[1].color, color );
		ColorCopy( v[2].color, color );
		ColorCopy( v[3].color, color );

		v[0].texles_per_pixel[0]= v[0].texles_per_pixel[1]=
		v[1].texles_per_pixel[0]= v[1].texles_per_pixel[1]=
		v[2].texles_per_pixel[0]= v[2].texles_per_pixel[1]=
		v[3].texles_per_pixel[0]= v[3].texles_per_pixel[1]= texels_per_pixel;

		x+= dx;
		v+= 4;
		str++;
	}
	vertex_buffer_pos_= v - vertices_.data();
}

void r_Text::AddTextPixelCoords( float x, float y, float size, const unsigned char* color, const char* text )
{
	const char* str= text;

	float x0;
	float dx, dy;

	x= 2.0f * x / viewport_width_ - 1.0f;
	y= -2.0f * y / viewport_height_ + 1.0f;

	x0= x;

	dx= 2.0f * size * float(letter_width_) / ( viewport_width_ * float(letter_height_) );
	dy= 2.0f * size / viewport_height_ ;

	y-= dy;

	unsigned char texels_per_pixel= (unsigned char)( 16.0f * letter_height_ / size );

	r_TextVertex* v= vertices_.data() + vertex_buffer_pos_;
	while( *str != 0 )
	{

		if( *str == '\n' )
		{
			x= x0;
			y-=dy;
			str++;
			continue;
		}

		int symb_pos= 95 - (*str-32);
		v[0].pos[0]= x;
		v[0].pos[1]= y;
		v[0].tex_coord[0]= 0;
		v[0].tex_coord[1]= symb_pos;

		v[1].pos[0]= x;
		v[1].pos[1]= y + dy;
		v[1].tex_coord[0]= 0;
		v[1].tex_coord[1]= symb_pos + 1;

		v[2].pos[0]= x + dx;
		v[2].pos[1]= y + dy;
		v[2].tex_coord[0]= 1;
		v[2].tex_coord[1]= symb_pos + 1;

		v[3].pos[0]= x + dx;
		v[3].pos[1]= y;
		v[3].tex_coord[0]= 1;
		v[3].tex_coord[1]= symb_pos;

		ColorCopy( v[0].color, color );
		ColorCopy( v[1].color, color );
		ColorCopy( v[2].color, color );
		ColorCopy( v[3].color, color );

		v[0].texles_per_pixel[0]= v[0].texles_per_pixel[1]=
		v[1].texles_per_pixel[0]= v[1].texles_per_pixel[1]=
		v[2].texles_per_pixel[0]= v[2].texles_per_pixel[1]=
		v[3].texles_per_pixel[0]= v[3].texles_per_pixel[1]= texels_per_pixel;


		x+= dx;
		v+= 4;
		str++;
	}
	vertex_buffer_pos_= v - vertices_.data();
}

void r_Text::Draw()
{
	font_texture_.Bind(0);

	text_vbo_.Bind();
	text_vbo_.VertexSubData( vertices_.data(), vertex_buffer_pos_ * sizeof(r_TextVertex), 0 );

	shader_.Bind();
	shader_.Uniform( "tex", 0 );
	shader_.Uniform( "inv_letters_in_texture", 1.0f / float(c_letters_in_texture_) );

	static const GLenum blend_func[]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, false, false,
		blend_func );
	r_OGLStateManager::UpdateState( state );

	glDrawElements( GL_TRIANGLES, vertex_buffer_pos_ * 6 / 4, GL_UNSIGNED_SHORT, nullptr );

	vertex_buffer_pos_= 0;
}

r_Text::r_Text( const char* font_file )
	: vertices_( c_text_buffer_size_ * 4 )
	, vertex_buffer_pos_(0)
{
	int v[4];
	glGetIntegerv( GL_VIEWPORT, v );
	viewport_width_ = float( v[2] );
	viewport_height_= float( v[3] );

	std::vector<unsigned short> quad_indeces( c_text_buffer_size_ * 6 );
	for( unsigned int i= 0, j= 0; i < c_text_buffer_size_ * 6; i+= 6, j+=4 )
	{
		quad_indeces[i]= j;
		quad_indeces[i + 1]= j + 1;
		quad_indeces[i + 2]= j + 2;

		quad_indeces[i + 3]= j + 2;
		quad_indeces[i + 4]= j + 3;
		quad_indeces[i + 5]= j;
	}

	text_vbo_.VertexData( nullptr, c_text_buffer_size_ * 4 * sizeof(r_TextVertex), sizeof(r_TextVertex) );
	text_vbo_.IndexData( quad_indeces.data(), c_text_buffer_size_ * 6 * sizeof(short), GL_UNSIGNED_SHORT, GL_TRIANGLES );

	{
		r_TextVertex v;
		int offset;
		offset= ((char*)v.pos) - ((char*)&v);
		text_vbo_.VertexAttribPointer( 0, 2, GL_FLOAT, false, offset );//vertex coordinates
		offset= ((char*)v.tex_coord) - ((char*)&v);
		text_vbo_.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, false, offset );//texture coordinates
		offset= ((char*)v.color) - ((char*)&v);
		text_vbo_.VertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE, true, offset );//color
		offset= ((char*)v.texles_per_pixel) - ((char*)&v);
		text_vbo_.VertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, offset );// texels per pixel
	}

	if( !shader_.Load( "shaders/text_frag.glsl", "shaders/text_vert.glsl" ) )
		h_Console::Error( "Text shader not found\n" );

	shader_.SetAttribLocation( "coord", 0 );
	shader_.SetAttribLocation( "tex_coord", 1 );
	shader_.SetAttribLocation( "color", 2 );
	shader_.SetAttribLocation( "texels_per_pixel", 3 );
	shader_.Create();

	r_ImgUtils::LoadTexture( &font_texture_, font_file );
	font_texture_.SetWrapMode( r_Texture::WrapMode::Clamp );

	letter_width_ = font_texture_.Width();
	letter_height_= font_texture_.Height() / c_letters_in_texture_;
}
