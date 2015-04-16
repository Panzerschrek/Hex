#version 330

uniform  sampler2D tex;

out vec4 color;

void main()
{
	float t = texture( tex, gl_PointCoord.xy).xyz;
	if( t < 0.05 )
		discard;
	color= vec4( 1.0, 1.0, 1.0, t * 0.5 );
}