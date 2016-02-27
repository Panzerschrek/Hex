#version 330

uniform sampler2D frame_buffer;
uniform sampler2D depth_buffer;
uniform float perspective_matrix_10;

out vec4 color;

float GetInvZ( ivec2 texel_coord )
{
	// inv_z= ( d - perspective_matrix_10 ) / perspective_matrix_14
	// But, we do not need "perspective_matrix_14", we need only negative sign
	float d= texelFetch( depth_buffer, texel_coord, 0 ).x * 2.0 - 1.0;
	float inv_z_scaled= perspective_matrix_10 - d;

	return inv_z_scaled;
}

void main()
{
	ivec2 texel_coord= ivec2( gl_FragCoord.xy );

	float inv_z[5];
	inv_z[0]= GetInvZ( texel_coord );
	inv_z[1]= GetInvZ( texel_coord + ivec2(+1,  0) );
	inv_z[2]= GetInvZ( texel_coord + ivec2(-1,  0) );
	inv_z[3]= GetInvZ( texel_coord + ivec2( 0, +1) );
	inv_z[4]= GetInvZ( texel_coord + ivec2( 0, -1) );

	// estimated accuracy of depth buffer
	const float depth_eps= 64.0 / 16777216.0;

	/*
	1 / z must be linear inside polygon
	detect pixels, where (1 / z)dx and (1 / z)dy both nonlinear
	and take sum of neighbor pixels
	*/
	float two_inv_z_0= 2.0 * inv_z[0];
	if( abs( inv_z[1] + inv_z[2] - two_inv_z_0 ) > depth_eps &&
		abs( inv_z[3] + inv_z[4] - two_inv_z_0 ) > depth_eps )
	{
		color= 0.2 * (
			texelFetch( frame_buffer, texel_coord, 0 ) +
			texelFetch( frame_buffer, texel_coord + ivec2(+1, 0), 0 ) +
			texelFetch( frame_buffer, texel_coord + ivec2(-1, 0), 0 ) +
			texelFetch( frame_buffer, texel_coord + ivec2( 0, 1), 0 ) +
			texelFetch( frame_buffer, texel_coord + ivec2( 0,-1), 0 ) );
	}
	else
		color= texelFetch( frame_buffer, texel_coord, 0 );
}
