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



r_PolygonBuffer::r_PolygonBuffer()
	: v_array_object(BUFFER_NOT_CREATED)
	, i_buffer(BUFFER_NOT_CREATED)
	, v_buffer(BUFFER_NOT_CREATED)
	, vertex_data_size(0)
	, vertex_size(0)
	, index_data_type(0)
	, index_data_size(0)
	, primitive_type(0)
{
}

r_PolygonBuffer::~r_PolygonBuffer()
{
    GLuint buf[2];
    int buf_size= 0;

    if( i_buffer != BUFFER_NOT_CREATED )
    {
        buf[ buf_size ] = i_buffer;
        buf_size++;
    }

    if( v_buffer != BUFFER_NOT_CREATED )
    {
        buf[ buf_size ] = v_buffer;
        buf_size++;
    }

    if( buf_size != 0 )
        glDeleteBuffers( buf_size, buf );

}

int r_PolygonBuffer::VertexData( const void* data, unsigned int d_size, unsigned int v_size )
{
	if( v_array_object == BUFFER_NOT_CREATED )
		glGenVertexArrays( 1, &v_array_object );
	glBindVertexArray( v_array_object );

    if( v_buffer == BUFFER_NOT_CREATED )
        glGenBuffers( 1, &v_buffer );
    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );

    glBufferData( GL_ARRAY_BUFFER, d_size, data, GL_STATIC_DRAW );

    vertex_data_size= d_size;
    vertex_size= v_size;

    return 0;
}

int  r_PolygonBuffer::VertexSubData( const void* data, unsigned int d_size, unsigned int shift )
{
    if( v_buffer == BUFFER_NOT_CREATED || v_array_object == BUFFER_NOT_CREATED )
        return 1;

	glBindVertexArray( v_array_object );
    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );

    glBufferSubData( GL_ARRAY_BUFFER, shift, d_size, data );
    return 0;
}

int r_PolygonBuffer::IndexData( const void* data, unsigned int size, GLenum d_type, GLenum p_type )
{
	if( v_array_object == BUFFER_NOT_CREATED )
		glGenVertexArrays( 1, &v_array_object );
	glBindVertexArray( v_array_object );

    if( i_buffer == BUFFER_NOT_CREATED  )
        glGenBuffers( 1, &i_buffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );

    glBufferData( GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );

    index_data_size= size;
    index_data_type= d_type;
    primitive_type= p_type;

    return 0;
}
int r_PolygonBuffer::IndexSubData( const void* data, unsigned int size, int shift )
{
    if( i_buffer== BUFFER_NOT_CREATED || v_array_object == BUFFER_NOT_CREATED  )
        return 1;

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );
    glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, shift, size, data );
    return 0;
}

void r_PolygonBuffer::Bind() const
{
    if( v_array_object != BUFFER_NOT_CREATED )
        glBindVertexArray( v_array_object );

	//if( i_buffer != BUFFER_NOT_CREATED )
	//	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer );
}

int r_PolygonBuffer::Show() const
{
    Bind();

    int s;
    if( i_buffer == BUFFER_NOT_CREATED )
    {
        if( v_buffer == BUFFER_NOT_CREATED )
            return 1;
        s= vertex_data_size / vertex_size;
        glDrawArrays( primitive_type, 0, s );
    }
    else
    {
        if( v_buffer == BUFFER_NOT_CREATED )
            return 1;
        s= index_data_type == GL_UNSIGNED_INT ? 4 : 2;
        s= index_data_size / s;
        glDrawElements( primitive_type, s, index_data_type, NULL );
    }
    return 0;
}


int	r_PolygonBuffer::VertexAttribPointer( int v_attrib, int components, GLenum type, bool normalize, int shift )
{
    if( v_array_object == BUFFER_NOT_CREATED || v_buffer == BUFFER_NOT_CREATED )
        return 1;

    glBindVertexArray( v_array_object );
    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );

	glEnableVertexAttribArray( v_attrib );
    glVertexAttribPointer( v_attrib, components, type, normalize, vertex_size, (void*) shift );
    return 0;
}

int r_PolygonBuffer::VertexAttribPointerInt( int v_attrib, int components, GLenum type, int shift )
{
   	if( v_array_object == BUFFER_NOT_CREATED || v_buffer == BUFFER_NOT_CREATED )
        return 1;

    glBindVertexArray( v_array_object );
    glBindBuffer( GL_ARRAY_BUFFER, v_buffer );

	glEnableVertexAttribArray( v_attrib );
    glVertexAttribIPointer( v_attrib, components, type, vertex_size, (void*) shift );
    return 0;
}

#endif//POLYGON_BUFFER_CPP
