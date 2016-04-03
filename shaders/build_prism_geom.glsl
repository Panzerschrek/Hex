#define STRIP_SIZE 0.02

layout( lines, invocations= 1) in;
layout( triangle_strip, max_vertices = 12 ) out;

in vec3 g_coord[];
in float g_alpha[];

uniform vec3 cam_pos;
uniform mat4 view_matrix;
out float f_alpha;

/*
out primitive:

   /\
  /  \
 /____\
|    /|
|   / |
|  /  |
| /   |
|_____|
 \    /
  \  /
   \/

*/
void main()
{
	vec3 edge_vec= normalize( g_coord[0] - g_coord[1] );
	vec3 norm= normalize( cross( cam_pos - g_coord[0], edge_vec ) );
	//vec3 norm= vec3( 0.0, 0.0, 1.0 );
	vec4 out_coord[6];

	out_coord[0]= view_matrix * vec4( g_coord[0] + (   norm - edge_vec ) * STRIP_SIZE, 1.0 );
	out_coord[1]= view_matrix * vec4( g_coord[0] + ( - norm - edge_vec ) * STRIP_SIZE, 1.0 );
	out_coord[2]= view_matrix * vec4( g_coord[1] + ( - norm + edge_vec ) * STRIP_SIZE, 1.0 );
	out_coord[3]= view_matrix * vec4( g_coord[1] + (   norm + edge_vec ) * STRIP_SIZE, 1.0 );
	out_coord[4]= view_matrix * vec4( g_coord[0], 1.0 );
	out_coord[5]= view_matrix * vec4( g_coord[1], 1.0 );

	f_alpha= g_alpha[0] * g_alpha[1];

	gl_Position= out_coord[0]; EmitVertex();
	gl_Position= out_coord[1]; EmitVertex();
	gl_Position= out_coord[2]; EmitVertex();
	EndPrimitive();
	gl_Position= out_coord[0]; EmitVertex();
	gl_Position= out_coord[2]; EmitVertex();
	gl_Position= out_coord[3]; EmitVertex();
	EndPrimitive();

	gl_Position= out_coord[0]; EmitVertex();
	gl_Position= out_coord[1]; EmitVertex();
	gl_Position= out_coord[4]; EmitVertex();
	EndPrimitive();
	gl_Position= out_coord[2]; EmitVertex();
	gl_Position= out_coord[3]; EmitVertex();
	gl_Position= out_coord[5]; EmitVertex();
	EndPrimitive();
}
