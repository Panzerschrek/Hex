#version 120

#define FILTER_TEXTURES
const vec3 sun_vector= normalize( vec3( 0.5, 0.9, 1.0 ) ); 

uniform sampler2D tex;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;

varying float f_color;
varying vec2 f_tex_coord;
varying vec2 f_tex_coord_shift;
varying vec3 f_normal;
varying vec2 f_light;

const float textures_in_atlas= 16.0;
const float inv_textures_in_atlas= 1.0/16.0;
const float inv_textures_in_atlas2= 1.0/256.0;
const float inv_atlas_texture_size= 1.0 / ( 256.0 * 16.0 );

void main()
{
	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color;

	vec2 tc= mod( f_tex_coord, inv_textures_in_atlas ) + f_tex_coord_shift;
	vec4 c= texture2D( tex, tc );
	if( c.a < 0.5 )
		discard;
	gl_FragColor= vec4(  l * c.xyz, 1.0 );
}