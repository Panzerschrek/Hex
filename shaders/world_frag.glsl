#version 400

uniform vec3 sun_vector;

uniform sampler2DArray tex;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;
uniform vec3 ambient_light_color;

in float f_color;
in vec3 f_tex_coord;
in vec2 f_light;

out vec4 color;

ivec2 GetHexagonTexCoordY( vec2 tex_coord )
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

vec4 SimpleFetch()
{
	return texture( tex, f_tex_coord );
}

vec4 HexagonFetch()
{
	float lod= textureQueryLod( tex, f_tex_coord.xy ).x;

	if( lod <= 0.5 )
	{
		ivec2 tex_size= textureSize( tex, 0 ).xy;
		int texture_layer= int(f_tex_coord.z+0.01);

		return texelFetch( tex, ivec3(
			mod( GetHexagonTexCoordY( f_tex_coord.xy * vec2(tex_size) ) ,tex_size ),
			texture_layer ), 0 );
	}
	else
		return SimpleFetch();
}

void main()
{
	vec4 c= HexagonFetch();
	if( c.a < 0.5 )
		discard;

	#ifdef LIGHTING_ONLY
	c.xyz= vec3( 0.5, 0.5, 0.5 );
	#endif

	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color + ambient_light_color;
	color= vec4( c.xyz * l, 1.0 );
}
