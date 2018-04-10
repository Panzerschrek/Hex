uniform sampler2D frame_buffer;
uniform sampler2D depth_buffer;

out vec4 color;

void main()
{
	ivec2 tex_coord= ivec2( gl_FragCoord.xy );

	const int radius= 2;
	float pixel_count= 0.0f;
	vec4 avg_color= vec4( 0.0, 0.0, 0.0, 0.0 );
	float min_depth= 1.0;

	int min_square_dist= radius * radius * 2;

	for( int dx= -radius; dx <= radius; ++dx )
	for( int dy= -radius; dy <= radius; ++dy )
	{
		int square_dist= dx * dx + dy * dy;
		if( square_dist > radius * radius )
			continue;
		if( square_dist > min_square_dist )
			continue;

		ivec2 tex_coord_current= tex_coord + ivec2( dx, dy );
		vec4 c= texelFetch( frame_buffer, tex_coord_current, 0 );
		if( c.a < 0.1 )
			continue;

		float d= texelFetch( depth_buffer, tex_coord_current, 0 ).x;

		if( square_dist < min_square_dist )
		{
			min_depth= min( min_depth, d );
			pixel_count= 1.0;
			avg_color= c;
			min_square_dist= square_dist;
		}
		else
		{
			pixel_count+= 1.0;
			avg_color+= c;
		}
	}

	color= avg_color / max( 1, pixel_count );
	gl_FragDepth= min_depth;
}
