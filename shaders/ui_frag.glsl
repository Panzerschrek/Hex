#version 330

noperspective in vec4 f_color;

out vec4 out_color;

void main(void)
{
	out_color= f_color;
	//out_color= vec4( 0.8, 0.3, 0.2, 0.5 );
}