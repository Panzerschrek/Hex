#version 120


const vec3 sun_vector= normalize( vec3( 0.5, 0.9, 1.0 ) ); 

uniform sampler2D tex;

varying float f_water_level;
varying vec3 f_normal;
varying vec2 f_tex_coord;
varying float f_light;

void main()
{
	float l= f_light * 1.25 + 0.05;//= max( 0.3, 1.5 * dot( f_normal, sun_vector ) );
	vec4 c=texture2D( tex, f_tex_coord );
	gl_FragColor= vec4( l * c.xyz, c.a );
}