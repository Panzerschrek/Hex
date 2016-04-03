in vec2 coord;
in vec4 color;

uniform mat4 transform_matrix;

noperspective out vec4 f_color;

void main(void)
{
	f_color= color;
	gl_Position= transform_matrix * vec4( coord ,0.0, 1.0 );
}
