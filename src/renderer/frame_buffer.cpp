#ifndef FRAME_BUFFER_CPP
#define FRAME_BUFFER_CPP
#include "frame_buffer.h"

#include "texture.h"
r_FrameBuffer* r_FrameBuffer::current_buffer= NULL;
int r_FrameBuffer::Create( int depth_bits,int stencil_bits, int color_textures_number, unsigned int* color_tex_components, GLuint* color_tex_data_types, int size_x, int size_y )
{

    glGenFramebuffers( 1, &id );
    glBindFramebuffer( GL_FRAMEBUFFER, id );

    int depth_type;
    if( depth_bits == 16 )
        depth_type= GL_DEPTH_COMPONENT16;
    else if( depth_bits == 24 )
        depth_type= GL_DEPTH_COMPONENT24;
    else if( depth_bits == 32 )
        depth_type= GL_DEPTH_COMPONENT32F;
    else
        depth_bits= 0;

	if( stencil_bits == 8 )
	{
		depth_type= GL_DEPTH24_STENCIL8;
		is_stencil= true;
	}

    if( depth_bits || stencil_bits )
    {
        this->depth_bits= depth_bits;
        glGenTextures( 1, &depth_texture_id );
        glBindTexture( GL_TEXTURE_2D, depth_texture_id );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
        glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_COMPARE_FUNC, GL_LESS );

        GLuint component_type= ( stencil_bits == 0 ) ? GL_DEPTH_COMPONENT : GL_DEPTH_STENCIL;
        GLuint d_type= ( stencil_bits == 0 ) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_INT_24_8;
        glTexImage2D( GL_TEXTURE_2D, 0, depth_type, size_x, size_y, 0, component_type, d_type, NULL );

		GLuint attach_type= ( stencil_bits == 0 ) ? GL_DEPTH_ATTACHMENT : GL_DEPTH_STENCIL_ATTACHMENT;
        glFramebufferTexture( GL_FRAMEBUFFER, attach_type, depth_texture_id, 0 );

        is_depth_texture= true;

    }


    int color_attachments[32];
    if( color_textures_number != 0 )
    {
        int components, d_type, component_type;
        glGenTextures( color_textures_number, color_textures_id );
        for( int i= 0; i< color_textures_number; i++ )
        {
            color_attachments[i]= GL_COLOR_ATTACHMENT0 + i;
            if( color_tex_components[i] == 3  &&  color_tex_data_types[i] == GL_UNSIGNED_BYTE )
            {
                components= GL_RGB8;
                component_type= GL_RGB;
                d_type= GL_UNSIGNED_BYTE;
            }
            else if( color_tex_components[i] == 4  &&  color_tex_data_types[i] == GL_UNSIGNED_BYTE )
            {
                components= GL_RGBA8;
                component_type= GL_RGBA;
                d_type= GL_UNSIGNED_BYTE;
            }
            else if( color_tex_components[i] == 3  &&  color_tex_data_types[i] == GL_FLOAT )
            {
                components= GL_RGB32F;
                component_type= GL_RGB;
                d_type= GL_FLOAT;
            }
            else if( color_tex_components[i] == 4  && color_tex_data_types[i] == GL_FLOAT )
            {
                components= GL_RGBA32F;
                component_type= GL_RGBA;
                d_type= GL_FLOAT;
            }
            else if( color_tex_components[i] == 3  && color_tex_data_types[i] == GL_HALF_FLOAT )
            {
                components= GL_RGB16F;
                component_type= GL_RGB;
                d_type= GL_HALF_FLOAT;
            }
            else if( color_tex_components[i] == 1  && color_tex_data_types[i] == GL_HALF_FLOAT )
            {
                components= GL_R16F;
                component_type= GL_RED;
                d_type= GL_HALF_FLOAT;
            }
            glBindTexture( GL_TEXTURE_2D, color_textures_id[i] );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

            glTexImage2D( GL_TEXTURE_2D, 0, components, size_x, size_y, 0, component_type, d_type, NULL );
            glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, color_textures_id[i], 0 );
        }
    }
    glDrawBuffers( color_textures_number,(const GLenum*) color_attachments );
    number_of_color_textures= color_textures_number;
    x_size= size_x;
    y_size= size_y;
    created= true;
    r_Texture::ResetBinding();
    return 0;
}

void r_FrameBuffer::Bind() const
{
    if( current_buffer != this )
    {
        current_buffer= (r_FrameBuffer*) this;
        glBindFramebuffer( GL_FRAMEBUFFER, id );
        glViewport( 0, 0, x_size, y_size );
    }
}

void r_FrameBuffer::BindColorTexture( unsigned int number, int unit )
{
    if( number >= number_of_color_textures )
        return;

    glActiveTexture( GL_TEXTURE0 + unit );
    glBindTexture( GL_TEXTURE_2D, color_textures_id[ number ] );

    r_Texture::ResetBinding();
}

void r_FrameBuffer::BindDepthTexture( unsigned int unit )
{
    glActiveTexture( GL_TEXTURE0 + unit );
    glBindTexture( GL_TEXTURE_2D, depth_texture_id );

    r_Texture::ResetBinding();
}

void r_FrameBuffer::BindNull()
{
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    current_buffer= NULL;
}

void r_FrameBuffer::SetDepthTextureFiltration( unsigned int min, unsigned int mag )
{
    if( is_depth_texture )
    {
        glBindTexture( GL_TEXTURE_2D, depth_texture_id );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min );

        r_Texture::ResetBinding();
    }
    return;
}
void r_FrameBuffer::SetColorTextureFiltration( unsigned int number, unsigned int min, unsigned int mag )
{
    if( number >= number_of_color_textures )
        return;

    glBindTexture( GL_TEXTURE_2D, color_textures_id[ number ] );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min );

    r_Texture::ResetBinding();
    return;

}

void r_FrameBuffer::ClearBuffer( bool clear_depth, bool clear_color  )
{
    Bind();
    int clear_func= 0;
    if( is_depth_texture && clear_depth )
        clear_func|= GL_DEPTH_BUFFER_BIT;

    if( number_of_color_textures != 0  && clear_color )
        clear_func|= GL_COLOR_BUFFER_BIT;

	if( is_stencil && clear_depth )
		clear_func|= GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

    glClear( clear_func );
}

void r_FrameBuffer::DesableDepthTextureCompareMode()
{
    glBindTexture( GL_TEXTURE_2D, depth_texture_id );
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_COMPARE_MODE,GL_NONE );
    r_Texture::ResetBinding();
}

r_FrameBuffer::r_FrameBuffer()
{
    created= false;
    depth_bits= 0;
    is_depth_texture= false;
    is_stencil= false;
}
r_FrameBuffer::~r_FrameBuffer()
{
    if( created )
    {
        if( is_depth_texture )
            glDeleteTextures( 1, &depth_texture_id );

        if( number_of_color_textures != 0 )
            glDeleteTextures( number_of_color_textures, color_textures_id );

        if( current_buffer == this )
            current_buffer= NULL;
    }
}

void r_FrameBuffer::SetDepthBuffer( r_FrameBuffer* depth_buffer )
{
    if( depth_buffer->depth_bits == 0 )
        return;
    Bind();
    glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_buffer->depth_texture_id, 0 );
    if( created && depth_bits != 0 )
    {
        glDeleteTextures( 1, &depth_texture_id );
    }
    depth_texture_id= depth_buffer->depth_texture_id;
    depth_bits= depth_buffer->depth_bits;
    is_depth_texture= true;

}

void r_FrameBuffer::DeattachDepthTexture()
{
    if( is_depth_texture )
    {
        Bind();
        glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 0, 0 );
    }
}
void r_FrameBuffer::ReattachDepthTexture()
{
    if( is_depth_texture )
    {
        Bind();
        glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture_id, 0 );
    }
}



#endif//FRAME_BUFFER_CPP
