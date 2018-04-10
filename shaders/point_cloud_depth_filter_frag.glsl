uniform sampler2D frame_buffer;
uniform sampler2D depth_buffer;
uniform float m10;
uniform float m14;

out vec4 color;

float DepthToLinearZ( float depth )
{
	return m14 * ( ( 2.0 * depth - 1.0 ) - m10 );
}

void main()
{
	ivec2 tex_coord= ivec2( gl_FragCoord.xy );

	vec4 c= texelFetch( frame_buffer, tex_coord, 0 );
	if( c.a < 0.1 )
	{
		color= vec4( 0.0, 0.0, 0.0, 0.0 );
		gl_FragDepth= 1.0;
		return;
	}

	float depth= texelFetch( depth_buffer, tex_coord, 0 ).x;
	float linear_z= DepthToLinearZ( depth );

	const int radius= 8;
	for( int dx= -radius; dx <= radius; ++dx )
	for( int dy= -radius; dy <= radius; ++dy )
	{
		int square_dist= dx * dx + dy * dy;
		if( square_dist > radius * radius || square_dist == 0 )
			continue;

		ivec2 tex_coord_current= tex_coord + ivec2( dx, dy );
		vec4 c= texelFetch( frame_buffer, tex_coord_current, 0 );
		if( c.a < 0.1 )
			continue;

		float sample_linear_z= DepthToLinearZ( texelFetch( depth_buffer, tex_coord_current, 0 ).x );

		vec3 cone_vec= vec3( float(dx), float(dy), sample_linear_z - linear_z );
		if( cone_vec.z < 1.0e-12 )
			continue;

		float angle_tan= length( cone_vec.xy ) / cone_vec.z;
		if( angle_tan < 400000.0 )
		{
			color= vec4( 0.0, 0.0, 0.0, 0.0 );
			gl_FragDepth= 1.0;
			return;
		}
	}

	color= c;
	gl_FragDepth= depth;
}
