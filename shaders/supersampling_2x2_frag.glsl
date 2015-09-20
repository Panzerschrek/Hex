#version 330

uniform sampler2D frame_buffer;

out vec4 color;

void main()
{
	ivec2 texel_coord= ivec2( gl_FragCoord.xy ) * 2;
	color= 0.25 * (
		texelFetch( frame_buffer, texel_coord, 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 0, 1 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 1, 1 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 1, 0 ), 0 ) );
}
