#version 330

uniform mat4 view_matrix;
uniform vec3 cam_pos;

in vec3 coord;

out vec3 f_view_vec;

void main()
{
	f_view_vec= coord;
	gl_Position= view_matrix * vec4( coord + cam_pos, 1.0 );
}