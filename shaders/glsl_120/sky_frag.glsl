#version 120

varying vec3 f_view_vec;

void main()
{
	vec3 view_vec= normalize( f_view_vec );	
	float l= ( 3.0 - view_vec.z ) * 0.25;
	gl_FragColor= vec4( 1.0, 1.1, 1.5, 0.5 ) * l;
	gl_FragDepth= 1.0;
}