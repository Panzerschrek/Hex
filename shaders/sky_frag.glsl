#version 330

out vec4 color;

in vec3 f_view_vec;

void main()
{
	vec3 view_vec= normalize( f_view_vec );	
	float l= ( 3.0 - view_vec.z ) * 0.25;
	color= vec4( 1.0, 1.1, 1.5, 0.5 ) * l;
	gl_FragDepth= 1.0;
}