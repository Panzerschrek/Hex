#version 330

const float SMOOTH_EDGE= 0.15;
uniform sampler2D tex;

flat in vec4 f_color;
noperspective in vec2 f_tex_coord;

out vec4 out_color;

void main(void)
{
	vec2 tex_coord_pixels= f_tex_coord *vec2( textureSize(tex, 0) );
	vec2 ddx= dFdx( tex_coord_pixels );
	vec2 ddy= dFdy( tex_coord_pixels );
	float texels_per_pixel= sqrt( max(dot(ddx, ddx), dot(ddy, ddy)) );

	// x - SDF, y - simple smooth font
	vec2 tex_value= texture( tex, f_tex_coord ).xy;

	float smooth_range= min( texels_per_pixel * SMOOTH_EDGE, 0.5 );
	float c= smoothstep( 0.5 - smooth_range, 0.5 + smooth_range, tex_value.x );

	// if we draw downscale text, blend SDF-based font with simple font
	float font_tye_blend_k= smoothstep( 1.0, 2.0, texels_per_pixel );
	c= mix( c, tex_value.y, font_tye_blend_k );

	out_color= vec4( f_color.xyz, c * f_color.a );
}
