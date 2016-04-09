uniform mat4 view_matrix;

in vec3 coord;

void main()
{
	gl_Position= view_matrix * vec4( coord , 1.0 );
}
