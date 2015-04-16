#include <QImage>

#include "text.hpp"
#include "ogl_state_manager.hpp"
#include "img_utils.hpp"


const unsigned char r_Text::default_color[4]= {255, 255, 255, 255 };


static inline void hColorCopy( unsigned char* dst, unsigned const char* src )
{
	*((int*)dst)= *((const int*)src);
}

void r_Text::AddMultiText( float colomn, float row, float size, const unsigned char* color, const char* text, ... )
{
	static char str[ H_MAX_TEXT_BUFFER_SIZE ];
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

	x0= x= d_size * colomn * float( letter_width ) / screen_x - 1.0f;
	y=  -d_size * (row + 1) * float( letter_height ) / screen_y + 1.0f;

    dx= (d_size * float(letter_width) ) / screen_x;
    dy= (d_size * float(letter_height) ) / screen_y;

	unsigned char texels_per_pixel= (unsigned char)( 16.0f / size );

    r_TextVertex* v= vertices + vertex_buffer_pos;
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

		hColorCopy( v[0].color, color );
		hColorCopy( v[1].color, color );
		hColorCopy( v[2].color, color );
        hColorCopy( v[3].color, color );

		v[0].texles_per_pixel[0]= v[0].texles_per_pixel[1]=
		v[1].texles_per_pixel[0]= v[1].texles_per_pixel[1]=
		v[2].texles_per_pixel[0]= v[2].texles_per_pixel[1]=
		v[3].texles_per_pixel[0]= v[3].texles_per_pixel[1]= texels_per_pixel;


        x+= dx;
        v+= 4;
        str++;
    }
    vertex_buffer_pos= v - vertices;
}

void r_Text::AddTextPixelCoords( float x, float y, float size, const unsigned char* color, const char* text )
{
	const char* str= text;

    float x0;
    float dx, dy;

	x= 2.0f * x / screen_x - 1.0f;
	y= -2.0f * y / screen_y + 1.0f;

	x0= x;

    dx= 2.0f * size * float(letter_width) / ( screen_x * float(letter_height) );
    dy= 2.0f * size / screen_y ;

    y-= dy;

	unsigned char texels_per_pixel= (unsigned char)( 16.0f * letter_height / size );

    r_TextVertex* v= vertices + vertex_buffer_pos;
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

		hColorCopy( v[0].color, color );
		hColorCopy( v[1].color, color );
		hColorCopy( v[2].color, color );
        hColorCopy( v[3].color, color );

		v[0].texles_per_pixel[0]= v[0].texles_per_pixel[1]=
		v[1].texles_per_pixel[0]= v[1].texles_per_pixel[1]=
		v[2].texles_per_pixel[0]= v[2].texles_per_pixel[1]=
		v[3].texles_per_pixel[0]= v[3].texles_per_pixel[1]= texels_per_pixel;


        x+= dx;
        v+= 4;
        str++;
    }
    vertex_buffer_pos= v - vertices;
}

void r_Text::Draw()
{
    font_texture.Bind(0);

    text_vbo.Bind();
    text_vbo.VertexSubData( vertices, vertex_buffer_pos * sizeof(r_TextVertex), 0 );

    text_shader.Bind();
    text_shader.Uniform( "tex", 0 );
    {
		float inv_letters_in_texture= 1.0f / float(LETTERS_IN_TEXTURE);
		text_shader.Uniform( "inv_letters_in_texture", inv_letters_in_texture );
    }

    r_OGLStateManager::BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	r_OGLStateManager::EnableBlend();
	r_OGLStateManager::DisableDepthTest();

    glDrawElements( GL_TRIANGLES, vertex_buffer_pos * 6 / 4, GL_UNSIGNED_SHORT, NULL );

    vertex_buffer_pos= 0;
}


r_Text::r_Text( const char* font_file ):
    vertex_buffer_pos(0)
{
    vertices= new  r_TextVertex[ H_MAX_TEXT_BUFFER_SIZE * 4 ];

    int v[4];
    glGetIntegerv( GL_VIEWPORT, v );
    screen_x= float( v[2] );
    screen_y= float( v[3] );

    unsigned short* quad_indeces= new unsigned short[H_MAX_TEXT_BUFFER_SIZE*6];
    for( unsigned int i= 0, j= 0; i< H_MAX_TEXT_BUFFER_SIZE*6; i+= 6, j+=4 )
    {
        quad_indeces[i]= j;
        quad_indeces[i + 1]= j + 1;
        quad_indeces[i + 2]= j + 2;

        quad_indeces[i + 3]= j + 2;
        quad_indeces[i + 4]= j + 3;
        quad_indeces[i + 5]= j;
    }

    text_vbo.VertexData( NULL, H_MAX_TEXT_BUFFER_SIZE * 4 * sizeof(r_TextVertex),
                         sizeof(r_TextVertex) );
    text_vbo.IndexData( quad_indeces, H_MAX_TEXT_BUFFER_SIZE * 6 * sizeof(short), GL_UNSIGNED_SHORT, GL_TRIANGLES );
    delete[] quad_indeces;

	{
		r_TextVertex v;
		int offset;
		offset= ((char*)v.pos) - ((char*)&v);
		text_vbo.VertexAttribPointer( 0, 2, GL_FLOAT, false, offset );//vertex coordinates
		offset= ((char*)v.tex_coord) - ((char*)&v);
		text_vbo.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, false, offset );//texture coordinates
		offset= ((char*)v.color) - ((char*)&v);
		text_vbo.VertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE, true, offset );//color
		offset= ((char*)v.texles_per_pixel) - ((char*)&v);
		text_vbo.VertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, offset );// texels per pixel
	}


	if( !text_shader.Load( "shaders/text_frag.glsl", "shaders/text_vert.glsl", NULL ) )
		printf( "error, text shader not found\n" );
    text_shader.SetAttribLocation( "coord", 0 );
    text_shader.SetAttribLocation( "tex_coord", 1 );
    text_shader.SetAttribLocation( "color", 2 );
    text_shader.SetAttribLocation( "texels_per_pixel", 3 );
    text_shader.Create();

	{
		QImage img( font_file );
		if( img.format() != img.Format_ARGB32 )
			img= img.convertToFormat( QImage::Format_ARGB32 );

		unsigned char* tex_data= (unsigned char*) img.constBits();
		r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( tex_data, img.width(), img.height() );
		font_texture.Create( r_FramebufferTexture::FORMAT_RGBA8, img.width(), img.height(), tex_data );
		font_texture.BuildMips();
		font_texture.SetFiltration( r_FramebufferTexture::FILTRATION_LINEAR_MIPMAP_LINEAR, r_FramebufferTexture::FILTRATION_LINEAR );
		font_texture.SetWrapMode( r_FramebufferTexture::WRAP_MODE_CLAMP );
	}
    letter_height= font_texture.Height() / LETTERS_IN_TEXTURE;
    letter_width= font_texture.Width();
}
