#include "world_common.glsl"

uniform sampler2D tex;
uniform float clouds_edge;
uniform vec3 clouds_color;

in vec2 f_tex_coord;
in vec2 f_relative_coord;

out vec4 color;

void main()
{
	const float c_edge= 0.1;
	float t= HexagonFetch( tex, f_tex_coord ).x;
	float a= smoothstep( clouds_edge - c_edge, clouds_edge + c_edge, t );

	float r= length(f_relative_coord);
	float ra= 1.0 - smoothstep( 0.5, 1.0, r );

	color= vec4( clouds_color * ( t * 0.2 + 0.8 ), a * ra );
}
