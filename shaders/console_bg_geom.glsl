#version 330

uniform vec3 screen_size;
uniform float pos;

layout( points, invocations= 1) in;
layout( triangle_strip, max_vertices = 6 ) out;

out vec2 tex_coord;

void main()
{
	float aspect= sqrt(3.0) * screen_size.x / screen_size.y;
	
	gl_Position= vec4( -1.0, pos, -1.0, 1.0 ); tex_coord= vec2( 0.0, 0.0 ); 
	EmitVertex();
	gl_Position= vec4( -1.0, 1.0, -1.0, 1.0 ); tex_coord= vec2( 0.0, 1.0-pos ); 
	EmitVertex();
	gl_Position= vec4(  1.0, 1.0, -1.0, 1.0 ); tex_coord= vec2( aspect, 1.0-pos ); 
	EmitVertex();	
	EndPrimitive();

	gl_Position= vec4( -1.0, pos, -1.0, 1.0 ); tex_coord= vec2( 0.0, 0.0 ); 
	EmitVertex();
	gl_Position= vec4(  1.0, 1.0, -1.0, 1.0 ); tex_coord= vec2( aspect, 1.0-pos ); 
	EmitVertex();
	gl_Position= vec4(  1.0, pos, -1.0, 1.0 ); tex_coord= vec2( aspect, 0.0 ); 
	EmitVertex();	
	EndPrimitive();
}