uniform mat4 view_matrix;

in vec3 coord;

out vec3 f_view_vec;

void main()
{
	f_view_vec= coord;
	gl_Position= view_matrix * vec4( coord, 1.0 );
}
