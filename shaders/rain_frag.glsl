uniform  sampler2D tex;
uniform vec3 light;

out vec4 color;

void main()
{
	float t = texture( tex, gl_PointCoord.xy).x;
	if( t < 0.05 )
		discard;

	const vec3 c_particle_color= vec3( 0.7, 0.7, 0.7 );

	color= vec4( c_particle_color * light, t * 0.4 );
}
