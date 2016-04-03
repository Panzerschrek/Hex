uniform sampler2D frame_buffer;

flat in vec2 f_tex_coord;

out vec4 color;

void main()
{
	ivec2 texel_coord= ivec2( gl_FragCoord.xy ) * 3;
	color= 0.1111111111111111 * (
		texelFetch( frame_buffer, texel_coord, 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 1, 0 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 2, 0 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 0, 1 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 1, 1 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 2, 1 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 0, 2 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 1, 2 ), 0 ) +
		texelFetch( frame_buffer, texel_coord + ivec2( 2, 2 ), 0 ) );
}
