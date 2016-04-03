in vec3 coord;

uniform mat4 view_matrix;
uniform vec3 build_prism_pos= vec3( 0.0, 0.0, 0.0 );
uniform float active_vertices[12];

out vec3 g_coord;
out float g_alpha;

void main()
{
	gl_Position= view_matrix *
		vec4( coord * vec3( 0.2886751345, 0.5, 1.0 ) + build_prism_pos, 1.0 );
	g_coord= coord * vec3( 0.2886751345, 0.5, 1.0 ) + build_prism_pos;
	g_alpha= active_vertices[ gl_VertexID ];
}
