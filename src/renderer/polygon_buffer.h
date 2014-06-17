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
#ifndef POLYGON_BUFFER_H
#define POLYGON_BUFFER_H
#include "ph.h"

#define BUFFER_NOT_CREATED 0xffffffff
class r_PolygonBuffer
{
public:
    r_PolygonBuffer();
	~r_PolygonBuffer();

	int VertexData	( void* data, unsigned int d_size, unsigned int v_size  );
	int VertexSubData	( void* data, unsigned int d_size, unsigned int shift );

	int IndexData		( void* data, unsigned int size, GLenum d_type, GLenum p_type );
	int IndexSubData	( void* data, unsigned int size, int shift );
	int	VertexAttribPointer( int v_attrib, int components, GLenum type, bool normalize, int stride );
	int VertexAttribPointerInt( int v_attrib, int components, GLenum type, int stride ) const;

	int SetVertexBuffer( r_PolygonBuffer* buf );
	int SetIndexBuffer( r_PolygonBuffer* buf );
	int Show() const;

	void Bind() const;

	inline void SetPrimitiveType( GLenum t )
	{
		primitive_type= t;
	}

private:
	void GenVAO();
	unsigned int vertex_data_size;//размер данных в байтах
	unsigned int vertex_size;//размер в байтах одной вершины

	GLenum index_data_type;//тип данных индексов ( GL_UNSIGNED_INT / GL_UNSIGNED_SHORT )
	unsigned int index_data_size;
	GLenum primitive_type;

	bool is_array;//просто массив или индексированый приметив
	GLuint v_buffer, i_buffer;
	GLuint v_array_object;

	static r_PolygonBuffer* current_buffer;

};


#endif//POLYGON_BUFFER_H
