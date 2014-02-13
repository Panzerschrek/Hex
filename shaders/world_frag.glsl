#version 330


uniform vec3 sun_vector= normalize( vec3( 0.5, 0.9, 1.0 ) ); 
out vec4 color;

uniform sampler2DArray tex;

in float f_color;
in vec3 f_tex_coord;
in vec3 f_normal;
in float f_light;

void main()
{
	
	//float l= max( 0.3, 1.5 * dot( f_normal, sun_vector ) );
	float l= f_light * 1.25 + 0.05;
	vec4 c= texture( tex, f_tex_coord );
	if( c.a < 0.5 )
		discard;
	//c.xyz= vec3( 0.7, 0.7, 0.7 );
	color= vec4( c.xyz * l, 1.0 );
}