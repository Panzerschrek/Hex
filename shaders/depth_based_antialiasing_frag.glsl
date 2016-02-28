#version 330

uniform sampler2D frame_buffer;
uniform sampler2D depth_buffer;

noperspective in vec2 f_tex_coord;

out vec4 color;

void main()
{
	vec2 texel_step= vec2(1.0, 1.0) / vec2( textureSize( frame_buffer, 0 ) );

	float d[5];
	d[0]= texture( depth_buffer, f_tex_coord ).x;
	d[1]= texture( depth_buffer, f_tex_coord + vec2(+texel_step.x,  0) ).x;
	d[2]= texture( depth_buffer, f_tex_coord + vec2(-texel_step.x,  0) ).x;
	d[3]= texture( depth_buffer, f_tex_coord + vec2( 0, +texel_step.y) ).x;
	d[4]= texture( depth_buffer, f_tex_coord + vec2( 0, -texel_step.y) ).x;

	// estimated accuracy of depth buffer
	const float depth_eps= 32.0 / 16777216.0;

	/*
	1 / z must be linear inside polygon
	detect pixels, where (1 / z)dx and (1 / z)dy both nonlinear
	and take sum of neighbor pixels
	*/
	float two_d_0= 2.0 * d[0];
	if( abs( d[1] + d[2] - two_d_0 ) > depth_eps &&
		abs( d[3] + d[4] - two_d_0 ) > depth_eps )
	{
		color= 0.2 * (
			texture( frame_buffer, f_tex_coord ) +
			texture( frame_buffer, f_tex_coord + vec2(+texel_step.x, 0) ) +
			texture( frame_buffer, f_tex_coord + vec2(-texel_step.x, 0) ) +
			texture( frame_buffer, f_tex_coord + vec2( 0, texel_step.y) ) +
			texture( frame_buffer, f_tex_coord + vec2( 0,-texel_step.y) ) );
	}
	else
		color= texture( frame_buffer, f_tex_coord );
}
