#version 330

out vec4 color;

in vec3 f_view_vec;

void main()
{
	vec3 view_vec= normalize( f_view_vec );	
	float l= ( 3.0 - view_vec.z ) * 0.25;
	color.xyz= vec3( 1.0, 1.1, 1.5 ) * l;
	color.a= 1.0;
	gl_FragDepth= 1.0;
}