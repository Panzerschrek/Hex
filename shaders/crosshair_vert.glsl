#version 330

const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, 1.0 ), vec2( 1.0, 1.0 ),
vec2( -1.0, -1.0 ), vec2( 1.0, -1.0 ), vec2( 1.0, 1.0 ) );

const vec2 tex_coord[6]= vec2[6](
vec2( 0.0, 0.0 ), vec2( 0.0, 1.0 ), vec2( 1.0, 1.0 ),
vec2( 0.0, 0.0 ), vec2( 1.0, 0.0 ), vec2( 1.0, 1.0 ) );

noperspective out vec2 f_tex_coord;

uniform mat4 transform_matrix;

void main()
{
	f_tex_coord= tex_coord[ gl_VertexID ];
	gl_Position= transform_matrix * vec4( coord[ gl_VertexID ], 0.0, 1.0 );
}
