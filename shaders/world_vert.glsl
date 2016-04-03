#ifndef TEX_SCALE_VECTOR
#error
#endif

uniform mat4 view_matrix;

in vec3 coord;
in vec3 tex_coord;
in vec2 light;

out vec3 f_tex_coord;
out float f_color;
out vec2 f_light;

void main()
{
	f_tex_coord= tex_coord * TEX_SCALE_VECTOR;
	f_light= light;
	gl_Position= view_matrix * vec4( coord , 1.0 );
}
