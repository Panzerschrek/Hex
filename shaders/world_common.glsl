const float c_hexagon_fetch_lod_edge= 0.5;

ivec2 GetHexagonTexCoordY( in vec2 tex_coord )
{
	vec2 tex_coord_dransformed;
	vec2 tex_coord_transformed_floor;
	ivec2 nearest_cell;
	vec2 d;

	tex_coord_dransformed.y= tex_coord.y;
	tex_coord_transformed_floor.y= floor( tex_coord_dransformed.y );
	nearest_cell.y= int( tex_coord_transformed_floor.y );
	d.y= tex_coord_dransformed.y - tex_coord_transformed_floor.y;

	tex_coord_dransformed.x= tex_coord.x - 0.5 * float( (nearest_cell.y^1) & 1 );
	tex_coord_transformed_floor.x= floor( tex_coord_dransformed.x );
	nearest_cell.x= int( tex_coord_transformed_floor.x );
	d.x= tex_coord_dransformed.x - tex_coord_transformed_floor.x;

	if( d.x > 0.5 + 1.5 * d.y )
		return ivec2( nearest_cell.x + ((nearest_cell.y^1)&1), nearest_cell.y - 1 );

	else
	if( d.x < 0.5 - 1.5 * d.y )
		return ivec2( nearest_cell.x - (nearest_cell.y&1), nearest_cell.y - 1 );

	else
		return nearest_cell;
}

vec4 HexagonFetch( in sampler2DArray tex, in vec3 tex_coord )
{
	float lod= textureQueryLod( tex, tex_coord.xy ).x;

	if( lod <= c_hexagon_fetch_lod_edge )
	{
		ivec2 tex_size= textureSize( tex, 0 ).xy;
		int texture_layer= int(tex_coord.z + 0.49);

		return texelFetch(
			tex,
			ivec3(
				mod( GetHexagonTexCoordY( tex_coord.xy * vec2(tex_size) ) ,tex_size ),
				texture_layer ),
			0 );
	}
	else
		return texture( tex, tex_coord );
}

vec4 HexagonFetch( in sampler2D tex, in vec2 tex_coord )
{
	float lod= textureQueryLod( tex, tex_coord ).x;

	if( lod <= c_hexagon_fetch_lod_edge )
	{
		ivec2 tex_size= textureSize( tex, 0 );

		return texelFetch(
			tex,
			ivec2( mod( GetHexagonTexCoordY( tex_coord * vec2(tex_size) ) ,tex_size ) ),
			0 );
	}
	else
		return texture( tex, tex_coord );
}

vec3 CombineLight( in vec3 normalized_sun_light, in vec3 normalized_fire_light, in vec3 normalized_ambient_light )
{
#if 0
	return normalized_sun_light + normalized_fire_light + normalized_ambient_light;
#else
	const vec3 c_one= vec3(1.0, 1.0, 1.0);
	const float c_mul= 0.28;
	return
		normalized_sun_light  * ( c_one - c_mul * normalized_fire_light ) +
		normalized_fire_light * ( c_one - c_mul * normalized_sun_light  ) +
		normalized_ambient_light;
#endif
}
