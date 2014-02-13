#version 120

#ifndef INV_MAX_WATER_LEVEL
#define INV_MAX_WATER_LEVEL 0.0625
#endif

#ifndef LIGHT_MULTIPLER
#define LIGHT_MULTIPLER 0.125
#endif

uniform mat4 view_matrix;

const vec3 normals[8]= vec3[8](
vec3( 0.0, 1.0, 0.0 ), vec3( 0.0, -1.0, 0.0 ), 
vec3( 0.5, 0.866025, 0.0 ), vec3( -0.5, -0.866025, 0.0 ), 
vec3( -0.5, 0.866025, 0.0 ), vec3( 0.5, -0.866025, 0.0 ), 
vec3( 0.0, 0.0, 1.0 ), vec3( 0.0, 0.0, -1.0 ) );

attribute vec3 coord;
attribute float water_depth;
attribute vec2 light;

varying float f_water_level;
varying vec3 f_normal;
varying vec2 f_tex_coord;
varying float f_light;


void main()
{
	f_normal= vec3( 0.0, 0.0, 1.0 );
	f_water_level= water_depth;
	f_tex_coord= 0.25 * coord.xy * vec2( 0.25, 0.57735 );
	f_light= light.x * LIGHT_MULTIPLER;
	gl_Position= view_matrix * vec4( coord, 1.0 );
}