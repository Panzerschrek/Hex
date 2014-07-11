#version 330
out vec4 color;
uniform sampler2D tex;
in vec2 tex_coord;
void main()
{
	color= vec4( texture( tex, tex_coord ).xyz, 0.8 );
}