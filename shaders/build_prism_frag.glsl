#version 330

out vec4 color;

in float f_alpha;

void main()
{
	color= vec4( 0.0, 0.0, 0.0, f_alpha );
}
