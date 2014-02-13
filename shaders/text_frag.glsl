#version 330
in vec4 f_color;
in vec2 f_tex_coord;
out vec4 out_color;
uniform sampler2D tex;
void main(void)
{
	float c= texture( tex, f_tex_coord ).x;
	out_color=vec4( f_color.xyz * c, max( f_color.a, c ) );
}