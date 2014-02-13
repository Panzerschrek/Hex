#version 330


const vec3 sun_vector= normalize( vec3( 0.5, 0.9, 1.0 ) ); 
out vec4 color;

uniform sampler2D tex;


in float f_water_level;
in vec3 f_normal;
in vec2 f_tex_coord;
in float f_light;

uniform float time;

void main()
{
	
	float l=  f_light * 1.25 + 0.05;
	//max( 0.3, 1.5 * dot( f_normal, sun_vector ) );

	vec2 tc= f_tex_coord + sin( f_tex_coord.yx * 8.0 + vec2( time, time ) ) * 0.06125;
	vec4 c=texture( tex, tc);
	//c= vec4( 1.0, 1.0, 1.0, 1.0 );
	color= vec4( l * c.xyz, c.a );
}