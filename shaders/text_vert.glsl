#version 330
in vec2 coord;
in vec2 tex_coord;
in vec4 color;
flat out vec4 f_color;
noperspective out vec2 f_tex_coord;
void main(void)
{
	f_color= color;
	f_tex_coord= tex_coord * vec2( 1.0/96.0 ,1.0 );
	gl_Position= vec4( coord ,0.0, 1.0 );
}