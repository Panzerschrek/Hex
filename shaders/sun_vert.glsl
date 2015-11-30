#version 330

#ifndef SUN_SIZE
#error
#endif

uniform vec3 sun_vector;
uniform vec3 cam_pos;
uniform mat4 view_matrix;

void main()
{
	gl_Position= view_matrix * vec4( sun_vector + cam_pos, 1.0 );
	gl_PointSize= SUN_SIZE;
}
