#version 330

#ifndef TEX_SCALE_VECTOR
#define TEX_SCALE_VECTOR vec3( 0.0625, 0.125 * 0.86602540, 1.0 )
#endif


uniform mat4 view_matrix;

const vec3 normals[8]= vec3[8](
vec3( 0.0, 1.0, 0.0 ), vec3( 0.0, -1.0, 0.0 ), 
vec3( 0.866025, 0.5, 0.0 ), vec3( -0.866025, -0.5, 0.0 ), 
vec3( -0.866025, 0.5, 0.0 ), vec3( 0.866025, -0.5, 0.0 ), 
vec3( 0.0, 0.0, 1.0 ), vec3( 0.0, 0.0, -1.0 ) );

const float block_side_light_k[8]= float[8](
1.0, 0.94, 0.94, 1.0, 0.94, 1.0, 0.97, 0.97 );

in vec3 coord;
in vec3 tex_coord;
in uint normal;
//in vec4 reserved;
in vec2 light;

out vec3 f_tex_coord;
out float f_color;
out vec3 f_normal;
out vec2 f_light;

void main()
{
	f_normal= normals[ normal ];
	f_tex_coord= tex_coord * TEX_SCALE_VECTOR;
	f_light= light;
	gl_Position= view_matrix * vec4( coord , 1.0 );
}