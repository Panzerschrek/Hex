uniform mat4 mat;
uniform vec2 pos;
uniform vec2 clouds_shift;
uniform float clouds_size;

const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, 1.0 ), vec2( 1.0, 1.0 ),
vec2( -1.0, -1.0 ), vec2( 1.0, -1.0 ), vec2( 1.0, 1.0 ) );

out vec2 f_tex_coord;
out vec2 f_relative_coord;

void main()
{
	const float c_clouds_height= 256.0;
	const float c_clouds_texture_scale= 1.0 / 4096.0;
	const vec2 c_tex_stretch= vec2( sqrt(3.0) * 0.5, 1.0 );

	f_relative_coord= coord[ gl_VertexID ];

	vec3 world_pos= vec3( coord[ gl_VertexID ], 0.0 ) * clouds_size + vec3( pos, c_clouds_height );
	f_tex_coord= ( world_pos.xy + clouds_shift ) * ( c_tex_stretch * c_clouds_texture_scale );

	gl_Position= mat * vec4( world_pos, 1.0 );
}
