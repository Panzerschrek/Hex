#version 330

uniform mat4 view_matrix;
uniform float time;

const vec3 normals[8]= vec3[8](
vec3( 0.0, 1.0, 0.0 ), vec3( 0.0, -1.0, 0.0 ), 
vec3( 0.866025, 0.5, 0.0 ), vec3( -0.866025, -0.5, 0.0 ), 
vec3( -0.866025, 0.5, 0.0 ), vec3( 0.866025, -0.5, 0.0 ), 
vec3( 0.0, 0.0, 1.0 ), vec3( 0.0, 0.0, -1.0 ) );


in vec3 coord;
in vec2 light;

out vec3 f_normal;
out vec2 f_tex_coord;
out vec2 f_light;

void main()
{
	f_normal= vec3( 0.0, 0.0, 1.0 );
	f_light= light;
	f_tex_coord= 0.25 * coord.xy * vec2( 0.25, 0.57735 );
	gl_Position= view_matrix * vec4( coord, 1.0 );
}