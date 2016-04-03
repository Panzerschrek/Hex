uniform vec3 sun_vector;

uniform sampler2D tex;
uniform float time;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;
uniform vec3 ambient_light_color;

in vec3 f_normal;
in vec2 f_tex_coord;
in vec2 f_light;

out vec4 color;

void main()
{
	// TODO - remove lighting copy-paste in shaders
#if 0 // Old code - just add fire and sun/
	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color + ambient_light_color;

#else // New code - reduce one source, if outher source is big.
	vec3 s= f_light.x * sun_light_color;
	vec3 f= f_light.y * fire_light_color;
	const vec3 c_one= vec3(1.0, 1.0, 1.0);
	const float c_mul= 0.28;
	vec3 l= s * ( c_one - c_mul * f ) + f * ( c_one - c_mul * s ) + ambient_light_color;

#endif

	vec2 tc= f_tex_coord + sin( f_tex_coord.yx * 8.0 + vec2( time, time ) ) * 0.06125;
	vec4 c= texture( tex, tc );

	color= vec4( l * c.xyz, c.a );
}
