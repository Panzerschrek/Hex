#version 330

uniform mat4 view_matrix;

in vec3 coord;
in vec2 birghtness_spectre;

out vec2 f_brightness_spectre;

void main()
{
	f_brightness_spectre= birghtness_spectre;
	gl_Position= view_matrix * vec4( coord, 1.0 );
}
