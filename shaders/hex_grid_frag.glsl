#include "world_common.glsl"

uniform sampler2D frame_buffer;

noperspective in vec2 f_tex_coord;

out vec4 color;

void main()
{
	const int lod= 3;

	const float k= sqrt(3.0) / 2.0;
	vec2 tex_coord_convertex= vec2( ( f_tex_coord.x - 0.5 ) * k + 0.5, f_tex_coord.y );

	ivec2 tex_size= textureSize( frame_buffer, lod ).xy;

	color=
		texelFetch(
			frame_buffer,
			max( ivec2( GetHexagonTexCoordY( tex_coord_convertex * vec2(tex_size) ) ), ivec2( 0, 0 ) ),
			lod );
}
