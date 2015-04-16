#version 400

uniform float light_scale_k = 1.0;//for exponential lighting

uniform vec3 sun_vector;

uniform sampler2DArray tex;
uniform vec3 sun_light_color;
uniform vec3 fire_light_color;
uniform vec3 ambient_light_color;

in float f_color;
in vec3 f_tex_coord;
flat in vec3 f_normal;
in vec2 f_light;

out vec4 color;

const float H_SPACE_SCALE_VECTOR_X= 0.8660254037;
const float H_INV_SPACE_SCALE_VECTOR_X= 1.15470053;
const float H_HEXAGON_EDGE_SIZE= 0.5773502691;
const vec3 TEX_COORD_SCALER= vec3( 1.0, H_INV_SPACE_SCALE_VECTOR_X, 1.0001 );

ivec2 GetHexagonTexCoord( vec2 tex_coord )
{	
	tex_coord.xy= tex_coord.yx;

	ivec2 candidate_cells[3];

	candidate_cells[0].x= int( floor( tex_coord.x / H_SPACE_SCALE_VECTOR_X ) );
	float y_delta= 0.5 * float((candidate_cells[0].x+1)&1);
	candidate_cells[0].y= int( floor( tex_coord.y  - y_delta )  );

	candidate_cells[1].x= candidate_cells[0].x - 1;
	candidate_cells[1].y= candidate_cells[0].y - (candidate_cells[0].x&1);
	candidate_cells[2].x= candidate_cells[1].x;
	candidate_cells[2].y= candidate_cells[1].y + 1;

	vec2 center_pos[3];

	center_pos[0].x= float( candidate_cells[0].x ) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE;
	center_pos[0].y= float( candidate_cells[0].y ) + y_delta + 0.5;
	center_pos[1].x= center_pos[2].x= center_pos[0].x - H_SPACE_SCALE_VECTOR_X;
	center_pos[1].y= center_pos[0].y - 0.5;
	center_pos[2].y= center_pos[0].y + 0.5;

	int nearest_cell= 0;
	float dst_min= 256.0;
	for( int i= 0; i< 3; i++ )
	{
		vec2 v= center_pos[i] - tex_coord;
		float dst= dot(v,v);
		if( dst < dst_min )
		{
			dst_min= dst;
			nearest_cell= i;
		}
	}


	return candidate_cells[ nearest_cell ].yx;
}


ivec2 GetHexagonTexCoordY( vec2 tex_coord )
{	
	ivec2 candidate_cells[3];

	candidate_cells[0].y= int( floor( tex_coord.y / H_SPACE_SCALE_VECTOR_X ) );
	float x_delta= 0.5 * float((candidate_cells[0].y+1)&1);
	candidate_cells[0].x= int( floor( tex_coord.x  - x_delta )  );

	candidate_cells[1].y= candidate_cells[0].y - 1;
	candidate_cells[1].x= candidate_cells[0].x - (candidate_cells[0].y&1);
	candidate_cells[2].y= candidate_cells[1].y;
	candidate_cells[2].x= candidate_cells[1].x + 1;

	vec2 center_pos[3];

	center_pos[0].y= float( candidate_cells[0].y ) * H_SPACE_SCALE_VECTOR_X + H_HEXAGON_EDGE_SIZE;
	center_pos[0].x= float( candidate_cells[0].x ) + x_delta + 0.5;
	center_pos[1].y= center_pos[2].y= center_pos[0].y - H_SPACE_SCALE_VECTOR_X;
	center_pos[1].x= center_pos[0].x - 0.5;
	center_pos[2].x= center_pos[0].x + 0.5;

	int nearest_cell= 0;
	float dst_min= 256.0;
	for( int i= 0; i< 3; i++ )
	{
		vec2 v= center_pos[i] - tex_coord;
		float dst= dot(v,v);
		if( dst < dst_min )
		{
			dst_min= dst;
			nearest_cell= i;
		}
	}


	return candidate_cells[ nearest_cell ];

}


vec4 SimpleFetch()
{
	return texture( tex, f_tex_coord * TEX_COORD_SCALER );
}

vec4 HexagonFetch()
{
	float lod= textureQueryLod( tex, f_tex_coord.xy * TEX_COORD_SCALER.xy ).x;
	
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

	vec3 l= f_light.x * sun_light_color + f_light.y * fire_light_color + ambient_light_color;	
	color= vec4( c.xyz * l, 1.0 );
}