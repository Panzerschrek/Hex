#version 330

uniform float light_scale_k = 1.0;//for exponential lighting

uniform vec3 sun_vector;

uniform sampler2DArray tex;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;
uniform vec3 ambient_light_color;

in float f_color;
in vec3 f_tex_coord;
in vec3 f_normal;
in vec2 f_light;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );
	if( c.a < 0.5 )
		discard;

	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color + ambient_light_color;	
	color= vec4( c.xyz * l, 1.0 );
}