#version 120
varying vec4 f_color;
varying vec2 f_tex_coord;

uniform sampler2D tex;
void main(void)
{
	float c= texture2D( tex, f_tex_coord ).x;
	gl_FragColor =vec4( f_color.xyz * c, max( f_color.a, c ) );
}