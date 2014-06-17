/*
*This file is part of FREG.
*
*FREG is free software: you can redistribute it and/or modify
*it under the terms of the GNU General Public License as published by
*the Free Software Foundation, either version 3 of the License, or
*(at your option) any later version.
*
*FREG is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with FREG. If not, see <http://www.gnu.org/licenses/>.*/
#ifndef POLYGON_BUFFER_CPP
#define POLYGON_BUFFER_CPP

#include "polygon_buffer.h"


r_PolygonBuffer* r_PolygonBuffer::current_buffer= NULL;

r_PolygonBuffer::r_PolygonBuffer()
{
    v_array_object= i_buffer= v_buffer= BUFFER_NOT_CREATED;
    is_array= true;
}

void r_PolygonBuffer::GenVAO()
{
	glGenVertexArrays( 1, &v_array_object );
	glBindVertexArray( v_array_object );
}
r_PolygonBuffer::~r_PolygonBuffer()
{
    GLuint buf[2];
    int buf_size= 0;
    /*запихиваю идентификаторы буфферов в один массив чтобы обойтись одним вызовом функции glDeleteBuffers*/
    if( i_buffer != BUFFER_NOT_CREATED )
    {
        buf[ buf_size ] = i_buffer;
        buf_size ++;
    }

    if( v_buffer != BUFFER_NOT_CREATED )
    {
        buf[ buf_size ] = v_buffer;
        buf_size ++;
    }

    if( buf_size != 0 )
        glDeleteBuffers( buf_size, buf );//удаление буфферов

    if( current_buffer == this )
        current_buffer= NULL;
}

int  r_PolygonBuffer::VertexData( void* data, unsigned int d_size, unsigned int v_size )
{
	if( v_array_object == BUFFER_NOT_CREATED )
		GenVAO();
    if( v_buffer== BUFFER_NOT_CREATED )
        glGenBuffers( 1, &v_buffer );

    //Bind();
    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );
    glBufferData( GL_ARRAY_BUFFER, d_size, data, GL_STATIC_DRAW );

    vertex_data_size= d_size;
    vertex_size= v_size;

    //glBindVertexArray( v_array_object );
    //glVertexAttribPointer( v_attrib, v_size / sizeof( float ), GL_FLOAT, GL_FALSE, 0, 0 );
    //glEnableVertexAttribArray( v_attrib );

    return 0;
}
int  r_PolygonBuffer::VertexSubData( void* data, unsigned int d_size, unsigned int shift )
{

    if( v_buffer == BUFFER_NOT_CREATED )
        return 1;

    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );
    glBufferSubData( GL_ARRAY_BUFFER, shift, d_size, data );
    return 0;
}

int r_PolygonBuffer::IndexData( void* data, unsigned int size, GLenum d_type, GLenum p_type )
{
	if( v_array_object == BUFFER_NOT_CREATED )
		GenVAO();
    if( i_buffer== BUFFER_NOT_CREATED )
        glGenBuffers( 1, &i_buffer );

    //Bind();
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );

    index_data_size= size;
    index_data_type= d_type;
    is_array= false;
    primitive_type= p_type;

    return 0;
}
int r_PolygonBuffer::IndexSubData( void* data, unsigned int size, int shift )
{
    if( i_buffer== BUFFER_NOT_CREATED )
        return 1;

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );
    glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, shift, size, data );
    return 0;
}

void r_PolygonBuffer::Bind() const
{

#ifndef OGL21
    if( v_array_object != BUFFER_NOT_CREATED )
        glBindVertexArray( v_array_object );
#endif

   /* if( i_buffer != BUFFER_NOT_CREATED )
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );

	 if( v_buffer != BUFFER_NOT_CREATED )
        glBindBuffer( GL_ARRAY_BUFFER, v_buffer );*/
}

int r_PolygonBuffer::Show() const
{
    Bind();

    int s;
    if( is_array )
    {
        if( v_buffer == BUFFER_NOT_CREATED )
            return 1;
        s= vertex_data_size / vertex_size;
        glDrawArrays( primitive_type, 0, s );
    }
    else
    {
        if( v_buffer == BUFFER_NOT_CREATED  || i_buffer == BUFFER_NOT_CREATED )
            return 1;
        s= index_data_type == GL_UNSIGNED_INT ? 4 : 2;
        s= index_data_size / s;
        glDrawElements( primitive_type, s, index_data_type, NULL );
    }
    return 0;
}


int	r_PolygonBuffer::VertexAttribPointer( int v_attrib, int components, GLenum type, bool normalize, int shift )
{
    if( v_array_object == BUFFER_NOT_CREATED )
        GenVAO();

    glBindVertexArray( v_array_object );
    if( v_buffer == BUFFER_NOT_CREATED  || i_buffer == BUFFER_NOT_CREATED )
        return 1;
    glVertexAttribPointer( v_attrib, components, type, normalize, vertex_size, (void*) shift );
    glEnableVertexAttribArray( v_attrib );
    return 0;
}

int r_PolygonBuffer::VertexAttribPointerInt( int v_attrib, int components, GLenum type, int shift  ) const
{
    if( v_array_object == BUFFER_NOT_CREATED )
        glGenVertexArrays( 1, (GLuint*) &v_array_object );

    glBindVertexArray( v_array_object );
    if( v_buffer == BUFFER_NOT_CREATED  || i_buffer == BUFFER_NOT_CREATED )
        return 1;
    glVertexAttribIPointer( v_attrib, components, type, vertex_size, (void*) shift );
    glEnableVertexAttribArray( v_attrib );
    return 0;
}

int r_PolygonBuffer::SetVertexBuffer( r_PolygonBuffer* buf )
{
    if( buf->v_buffer == BUFFER_NOT_CREATED )
        return 1;

    if( v_buffer != BUFFER_NOT_CREATED )
        glDeleteBuffers( 1, &v_buffer );

    v_buffer= buf->v_buffer;
    buf->v_buffer= BUFFER_NOT_CREATED;

    return 0;
}
int r_PolygonBuffer::SetIndexBuffer( r_PolygonBuffer* buf )
{
    if( buf->i_buffer == BUFFER_NOT_CREATED )
        return 1;

    if( i_buffer != BUFFER_NOT_CREATED )
        glDeleteBuffers( 1, &i_buffer );

    i_buffer= buf->i_buffer;
    buf->i_buffer= BUFFER_NOT_CREATED;

    return 0;
}
#endif//POLYGON_BUFFER_CPP
