#version 330

uniform float inv_letters_in_texture;

in vec2 coord;
in vec2 tex_coord;
in vec4 color;
in vec2 texels_per_pixel;

flat out vec4 f_color;
flat out float f_texels_per_pixel;
noperspective out vec2 f_tex_coord;

void main(void)
{
	f_color= color;
	f_tex_coord= tex_coord * vec2( 1.0, inv_letters_in_texture );

	f_texels_per_pixel= max( texels_per_pixel.x, texels_per_pixel.y ) * (1.0/16.0);
	gl_Position= vec4( coord ,0.0, 1.0 );
}