#version 120

#ifndef TEX_SCALE_VECTOR
#define TEX_SCALE_VECTOR vec3( 0.0625, 0.125 * 0.86602540, 1.0 )
#endif

#ifndef LIGHT_MULTIPLER
#define LIGHT_MULTIPLER 0.125
#endif

uniform mat4 view_matrix;
const vec3 normals[8]= vec3[8](
vec3( 0.0, 1.0, 0.0 ), vec3( 0.0, -1.0, 0.0 ), 
vec3( 0.866025, 0.5, 0.0 ), vec3( -0.866025, -0.5, 0.0 ), 
vec3( -0.866025, 0.5, 0.0 ), vec3( 0.866025, -0.5, 0.0 ), 
vec3( 0.0, 0.0, 1.0 ), vec3( 0.0, 0.0, -1.0 ) );

const float block_side_light_k[8]= float[8](
1.0, 0.94, 0.94, 1.0, 0.94, 1.0, 0.97, 0.97 );

attribute vec3 coord;
attribute vec3 tex_coord;
attribute float normal;
attribute vec2 light;

varying vec2 f_tex_coord;
varying vec2 f_tex_coord_shift;
varying float f_color;
varying vec3 f_normal;
varying float f_light;

const float textures_in_atlas= 16.0;
const float inv_textures_in_atlas= 1.0/16.0;
const float inv_textures_in_atlas2= 1.0/256.0;
void main()
{
	int i_normal= int(normal);
	block_side_light_k[8];
	f_normal= normals[ i_normal  ];
	f_light= light.x * LIGHT_MULTIPLER * block_side_light_k[ i_normal ];

	f_tex_coord= ( tex_coord.xy  )* TEX_SCALE_VECTOR.xy * inv_textures_in_atlas;
	f_tex_coord_shift.x= mod( tex_coord.z, textures_in_atlas ) * inv_textures_in_atlas;
	f_tex_coord_shift.y= tex_coord.z * inv_textures_in_atlas2 - f_tex_coord_shift.x * inv_textures_in_atlas;
	gl_Position= view_matrix * vec4( coord, 1.0 );
}