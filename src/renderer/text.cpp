#include <QImage>

#include "text.hpp"
#include "ogl_state_manager.hpp"
#include "img_utils.hpp"


const unsigned char r_Text::default_color[4]= {255, 255, 255, 32 };
//r_Text* r_Text::default_text= NULL;



void r_Text::DrawCross()
{
    draw_crosshair= true;
    static const char t[]= { 127, 0 };
    static const unsigned char color[]= { 255, 255, 255, 0 };
    AddText( 0, 0, 2, color, t );
    draw_crosshair= false;

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
        v[0].pos[0]= x;
        v[0].pos[1]= y;
        v[0].tex_coord[0]= *str - 32;
        v[0].tex_coord[1]= 0;

        v[1].pos[0]= x;
        v[1].pos[1]= y + dy;
        v[1].tex_coord[0]= *str - 32;
        v[1].tex_coord[1]= 1;

        v[2].pos[0]= x + dx;
        v[2].pos[1]= y + dy;
        v[2].tex_coord[0]= *str - 32 + 1;
        v[2].tex_coord[1]= 1;

        v[3].pos[0]= x + dx;
        v[3].pos[1]= y;
        v[3].tex_coord[0]= *str - 32 + 1;
        v[3].tex_coord[1]= 0;

        for( unsigned int i= 0; i< 4; i++ )
            *((int*)v[i].color)= *((int*)color);//copy 4 bytes per one asm command

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

    r_OGLStateManager::BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glEnable( GL_BLEND );
	//glDisable( GL_DEPTH_TEST );
	r_OGLStateManager::EnableBlend();
	r_OGLStateManager::DisableDepthTest();

    glDrawElements( GL_TRIANGLES, vertex_buffer_pos * 6 / 4, GL_UNSIGNED_SHORT, NULL );

    vertex_buffer_pos= 0;
}


r_Text::r_Text( const char* font_file ):
    vertex_buffer_pos(0),
    draw_crosshair(false)
{
//    default_text= this;

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

    text_vbo.VertexAttribPointer( 0, 2, GL_FLOAT, false, 0 );//vertex coordinates
    text_vbo.VertexAttribPointer( 1, 2, GL_UNSIGNED_SHORT, false, sizeof(float)*2 );//texture coordinates
    text_vbo.VertexAttribPointer( 2, 4, GL_UNSIGNED_BYTE, true, sizeof(float)*2 + 2*sizeof(short) );//color


	if( text_shader.Load( "shaders/text_frag.glsl", "shaders/text_vert.glsl", NULL ) )
		printf( "error, text shader not found\n" );
    text_shader.SetAttribLocation( "coord", 0 );
    text_shader.SetAttribLocation( "tex_coord", 1 );
    text_shader.SetAttribLocation( "color", 2 );
    text_shader.MoveOnGPU();

	{
		QImage img( font_file );
		if( img.format() != img.Format_ARGB32 )
			img= img.convertToFormat( QImage::Format_ARGB32 );

		unsigned char* tex_data= (unsigned char*) img.constBits();
		r_ImgUtils::RGBA8_MirrorVerticalAndSwapRB( tex_data, img.width(), img.height() );
		font_texture.Create( r_FramebufferTexture::FORMAT_RGBA8, img.width(), img.height(), tex_data );
		font_texture.BuildMips();
		font_texture.SetFiltration( r_FramebufferTexture::FILTRATION_LINEAR_MIPMAP_LINEAR, r_FramebufferTexture::FILTRATION_LINEAR );
	}
    letter_height= font_texture.Height();
    letter_width= font_texture.Width() / LETTERS_IN_TEXTURE;
}
