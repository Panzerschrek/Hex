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
#ifndef GLSL_PROGRAM_CPP
#define GLSL_PROGRAM_CPP
#include "glsl_program.h"

r_GLSLProgram* r_GLSLProgram::current_prog= NULL;


const char* r_GLSLProgram::attrib_names[ MAX_NUMBER_OF_ATTRIBS ] =
{
    "position", "tex_coord", "normal",
    "binormal", "tangent"	 , "color",
    "user0", "user1", "user2", "user3", "user4", "user5"
};



r_GLSLProgram::r_GLSLProgram()
{
    geom_text[0]= frag_text[0]= vert_text[0]= 0;//зануление текстов програм
    attrib_num= 0;
    uniform_num= 0;
    define_num= 0;
    vert_handle= frag_handle= geom_handle= prog_handle= HANDLE_NOT_CREATED;

    memset( frag_text, 0, MAX_SHADER_SRC_LEN  );
    memset( vert_text, 0, MAX_SHADER_SRC_LEN  );
    memset( geom_text, 0, MAX_SHADER_SRC_LEN  );
}

r_GLSLProgram::~r_GLSLProgram()
{
    if( prog_handle != HANDLE_NOT_CREATED )
    {
        glDeleteProgram( prog_handle );
    }

    if( frag_handle != HANDLE_NOT_CREATED )
        glDeleteShader( frag_handle );

    if( vert_handle != HANDLE_NOT_CREATED )
        glDeleteShader( vert_handle );

    if( geom_handle != HANDLE_NOT_CREATED )
        glDeleteShader( geom_handle );


    if( current_prog == this )
        current_prog= NULL;
}

void r_GLSLProgram::Bind() const
{
    if( this != current_prog )
    {
        /*без преобразования не работает, т. к. компилятору неведомо, что может случится с this*/
        current_prog= ( r_GLSLProgram* )this;

        if( prog_handle != HANDLE_NOT_CREATED )
            glUseProgram( prog_handle );
    }
}

int r_GLSLProgram::Define( const char* def )
{
    if( define_num == MAX_SHADER_DEFINES )
        return 1;

    sprintf( defines[ define_num ], "#define %s\n", def );
    define_num++;
    return 0;
}

int r_GLSLProgram::UnDefine( const char* def )
{
	char str[ MAX_DEFINE_LEN ];
	sprintf( str, "#define %s\n", def );
	for( unsigned int i= 0; i< define_num; i++ )
	{
		if( !strcmp( str, defines[i] ) )
		{
			if( i < define_num - 1 )
				strcpy( defines[i], defines[ define_num - 1 ] );
			define_num--;
			return 0;
		}
	}
	return 1;
}

int r_GLSLProgram::Load ( const char *frag_file, const char *vert_file, const char *geom_file )
{
    int f_size;
    FILE* file;
    if( frag_file != NULL )
    {
        file= fopen( frag_file, "rb" );
        if( file == NULL )
            return 1;

        fseek( file, 0, SEEK_END );
        f_size= ftell( file );
        fseek( file, 0, SEEK_SET );

        fread( frag_text, 1, f_size, file );
        fclose( file );
    }

    if( vert_file != NULL )
    {
        file= fopen( vert_file, "rb" );
        if( file == NULL )
            return 1;

        fseek( file, 0, SEEK_END );
        f_size= ftell( file );
        fseek( file, 0, SEEK_SET );

        fread( vert_text, 1, f_size, file );
        fclose( file );
    }

    if( geom_file != NULL )
    {
        file= fopen( geom_file, "rb" );
        if( file == NULL )
            return 1;

        fseek( file, 0, SEEK_END );
        f_size= ftell( file );
        fseek( file, 0, SEEK_SET );

        fread( geom_text, 1, f_size, file );
        fclose( file );
    }
    return 0;
}

void r_GLSLProgram::ShaderSource( const char* frag_src, const char* vert_src, const char* geom_src )
{
    if( frag_src!= NULL )
        strcpy( frag_text, frag_src );
    else
        frag_text[0]= 0;

    if( vert_src!= NULL )
        strcpy( vert_text, vert_src );
    else
        vert_text[0]= 0;

    if( geom_src!= NULL )
        strcpy( geom_text, geom_src );
    else
        geom_text[0]= 0;
}

int r_GLSLProgram::MoveOnGPU()
{
    int len;
    int compile_status;
    char build_log[4096];
    int result= 0;
    char* shader_text_buf[ 1 + MAX_SHADER_DEFINES + 1 ];
    unsigned int shader_strings_len[1 + MAX_SHADER_DEFINES + 1];
    int shader_tex_buf_pos= define_num;

    for( int i=0; i< define_num; i++ )
    {
        shader_text_buf[i]= defines[i];
        shader_strings_len[i]= strlen( defines[i] );
    }
    shader_text_buf[ define_num + 1 ] = NULL;
    shader_strings_len[ define_num + 1 ]= 0;

    prog_handle= glCreateProgram();

    if( frag_text[0]!= 0 )
    {
        frag_handle= glCreateShader( GL_FRAGMENT_SHADER );

        shader_strings_len[shader_tex_buf_pos ]= strlen (frag_text );
        shader_text_buf[shader_tex_buf_pos ]= frag_text;
        glShaderSource(frag_handle, shader_tex_buf_pos + 1, (const char**)shader_text_buf, ( const GLint*) shader_strings_len );

        glCompileShader( frag_handle );
        glGetShaderiv( frag_handle, GL_COMPILE_STATUS, &compile_status );
        if( !compile_status )
        {
            glGetShaderInfoLog( frag_handle, 1024, &len, build_log );
            printf( "fragment shader error:\n\n%s\nerrors:\n%s\n", frag_text, build_log );
            result= 1;
            compile_status= 1;
        }
        glAttachShader( prog_handle, frag_handle );

    }

    if( vert_text[0]!= 0 )
    {
        vert_handle= glCreateShader( GL_VERTEX_SHADER );

        shader_strings_len[shader_tex_buf_pos]= strlen( vert_text );
        shader_text_buf[shader_tex_buf_pos]= vert_text;
        glShaderSource(  vert_handle, shader_tex_buf_pos + 1, (const char**)shader_text_buf,  ( const GLint*)shader_strings_len );

        glCompileShader( vert_handle);
        glGetShaderiv( vert_handle, GL_COMPILE_STATUS, &compile_status );
        if( !compile_status )
        {
            glGetShaderInfoLog( vert_handle, 1024, &len, build_log );
            printf( "vertex shader error:\n\n%s\nerrors:\n%s\n", vert_text, build_log );
            result= 1;
            compile_status= 1;
        }
        glAttachShader( prog_handle, vert_handle );
    }

    if( geom_text[0]!= 0 )
    {
        geom_handle= glCreateShader( GL_GEOMETRY_SHADER );

        shader_strings_len[shader_tex_buf_pos]= strlen( geom_text );
        shader_text_buf[shader_tex_buf_pos]= geom_text;
        glShaderSource( geom_handle, shader_tex_buf_pos + 1,(const char**) shader_text_buf,  ( const GLint*) shader_strings_len );

        glCompileShader( geom_handle);
        glGetShaderiv( geom_handle, GL_COMPILE_STATUS, &compile_status );
        if( !compile_status )
        {
            glGetShaderInfoLog( geom_handle, 1024, &len, build_log );
            printf( "geometrical shader error:\n %s\n", build_log );
            result= 1;
            compile_status= 1;
        }
        glAttachShader( prog_handle, geom_handle );
    }


    for( int i= 0; i< attrib_num; i++ )
    {
        glBindAttribLocation( prog_handle, attribs[i], user_attribs[i] );
    }
    glLinkProgram( prog_handle );
    glGetProgramiv( prog_handle, GL_LINK_STATUS, &compile_status );
    if( ! compile_status )
    {
        glGetProgramInfoLog( prog_handle, 1024, &len, build_log );
        printf( "shader link error:\n %s\n", build_log );
        result= 1;
    }

    FindAllUniforms();

    return result;
}


int	r_GLSLProgram::FindAttrib  ( const char* name )
{
    if( attrib_num >= MAX_NUMBER_OF_ATTRIBS )
        return 1;

    //int a= glGetAttribLocation( prog_handle, name );

    //if( a < 0 )
    //	return 2;

    //attribs[ attrib_num ]= a;
    //strcpy( user_attribs[ attrib_num ], name );
    //attrib_num++;

    return 0;
}

int r_GLSLProgram::SetAttribLocation( const char* name, unsigned int location )
{
    /*unsigned int i;
    for( i= 0; i< attrib_num; i++ )
    {
        if( !strcmp( name, attrib_names[i] ) )
        {
            glBindAttribLocation( prog_handle, location, user_attribs[i] );
            return 0;
        }
    }
    return 1;*/
    if( attrib_num >= MAX_NUMBER_OF_ATTRIBS )
        return 1;

    strcpy( user_attribs[ attrib_num ], name );
    attribs[ attrib_num ]= location;
    attrib_num++;
    return 0;
}

int	r_GLSLProgram::FindUniform ( const char* name )
{
    int tmp= glGetUniformLocation( prog_handle, name );

    if( tmp == -1 )
    {
        //printf( "uniform \"%s\" not found.\n", name );for debugging
        return 1;
    }

    strcpy( uniforms_name[ uniform_num ] , name );
    uniforms[ uniform_num ]= tmp;
    uniform_num++;

    return 0;
}


bool IsCIdentiferCharacter( char c )
{
    return  ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_';
}

void r_GLSLProgram::FindAllUniformsInShader( const char* shader_text )
{
    unsigned int pos= 0;
    const char* uniform_str;
    char uniform_name[ MAX_UNIFORM_NAME_LEN ];
    unsigned int i;

    uniform_str= shader_text;
    while( uniform_str= strstr( uniform_str, "uniform " ) )
    {
        uniform_str+= 8;//+=strlen( "uniform " )
        while( IsCIdentiferCharacter( *uniform_str ) )//skip type of uniform
			uniform_str++;
		uniform_str++;

        i= 0;
        while( IsCIdentiferCharacter( uniform_str[i] )  )
        {
            uniform_name[i]= uniform_str[i];
            i++;
        }
        uniform_name[i]= 0;
        FindUniform( uniform_name );
    }
}

void r_GLSLProgram::FindAllUniforms()
{
    if( vert_text[0]!= 0  )
        FindAllUniformsInShader( vert_text );

    if( frag_text[0]!= 0  )
        FindAllUniformsInShader( frag_text );

    if( geom_text[0]!= 0  )
        FindAllUniformsInShader( geom_text );
}



int	r_GLSLProgram::GetUniform( const char* name ) const
{
    int i;
    for( i= 0; i< uniform_num; i++ )
    {
        if( ! strcmp( name, uniforms_name[i] ) )
            break;
    }
    if( i == uniform_num )
        return -1;

    return uniforms[i];
}


int	r_GLSLProgram::Uniform( const char* name, const m_Vec3& v) const
{
    int u= GetUniform( name );
    if( u == -1 )
        return 1;

    glUniform3f( u, v.x, v.y, v.z );
    return 0;
}

int	r_GLSLProgram::Uniform( const char* name, int i ) const
{
    int u= GetUniform( name );
    if( u == -1 )
        return 1;

    glUniform1i( u, i );

    return 0;
}

int	r_GLSLProgram::Uniform( const char* name, const m_Mat4& m ) const
{
    int u= GetUniform( name );
    if( u == -1 )
        return 1;

    /*GL_FALSE - означает что матрица, используемая мной транспонируется в OpenGL и всё будет збсь*/
    glUniformMatrix4fv( u, 1, GL_FALSE, m.value );
    return 0;
}

int	r_GLSLProgram::Uniform( const char* name, const m_Mat3& m ) const
{
    int u= GetUniform( name );
    if( u == -1 )
        return 1;

    /*GL_FALSE - означает что матрица, используемая мной транспонируется в OpenGL и всё будет збсь*/
    glUniformMatrix3fv( u, 1, GL_FALSE, m.value );
    return 0;
}


int	r_GLSLProgram::Uniform( const char* name, float f ) const
{
    int u= GetUniform( name );
    if( u == -1 )
        return 1;

    glUniform1f( u, f );
    return 0;
}

/*int	r_GLSLProgram::Uniform( int id, float f ) const
{
    if( id >= uniform_num )
        return 1;

    glUniform1f( uniforms[id], f );
    return 0;
}

int	r_GLSLProgram::GetUniformId	( const char* name )
{
    int i;
    for( i= 0; i< uniform_num; i++ )
    {
        if( ! strcmp( name, uniforms_name[i] ) )
            break;
    }
    if( i == uniform_num )
        return -1;

    return i;
}


int	r_GLSLProgram::Uniform( int id, m_Vec3& v) const
{
    if( id >= uniform_num )
        return 1;

    glUniform3f( uniforms[id], v.x, v.y, v.z );
    return 0;
}

int	r_GLSLProgram::Uniform( int id, int i ) const
{
    if( id >= uniform_num )
        return 1;

    glUniform1i( uniforms[id], i );
    return 0;
}

int	r_GLSLProgram::Uniform( int id, m_Mat4& m ) const
{
    if( id >= uniform_num )
        return 1;

    glUniformMatrix4fv( uniforms[id], 1, GL_FALSE, m.value );
    return 0;
}

int	r_GLSLProgram::Uniform( int id, m_Mat3& m ) const
{
    if( id >= uniform_num )
        return 1;

    glUniformMatrix3fv( uniforms[id], 1, GL_FALSE, m.value );
    return 0;

}
*/
#endif//GLSL_PROGRAM_CPP
