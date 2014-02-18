#version 120


const vec3 sun_vector= normalize( vec3( 0.5, 0.9, 1.0 ) ); 

uniform sampler2D tex;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;


varying float f_water_level;
varying vec3 f_normal;
varying vec2 f_tex_coord;
varying vec2 f_light;

void main()
{
	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color;
	vec4 c=texture2D( tex, f_tex_coord );
	gl_FragColor= vec4( l * c.xyz, c.a );
}