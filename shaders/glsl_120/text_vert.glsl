#version 120
attribute vec2 coord;
attribute vec2 tex_coord;
attribute vec4 color;
varying vec4 f_color;
varying vec2 f_tex_coord;
void main(void)
{
	f_color= color;
	f_tex_coord= tex_coord * vec2( 1.0/96.0 ,1.0 );
	gl_Position= vec4( coord ,0.0, 1.0 );
}