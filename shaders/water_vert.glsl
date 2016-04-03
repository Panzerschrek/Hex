uniform mat4 view_matrix;

in vec3 coord;
in vec2 light;

out vec3 f_normal;
out vec2 f_tex_coord;
out vec2 f_light;

void main()
{
	f_normal= vec3( 0.0, 0.0, 1.0 );
	f_light= light;
	f_tex_coord= coord.xy * TEX_SCALE_VECTOR;
	gl_Position= view_matrix * vec4( coord, 1.0 );
}
