uniform sampler2D tex;
uniform sampler2D spectre;

uniform float time;

in vec2 f_tex_coord_x_shift;
in float f_power;

out vec4 color;

void main()
{
	const vec2 c_tex_scale= vec2( 0.5, 0.33333 );
	vec2 tex_coord= c_tex_scale * ( gl_PointCoord + f_tex_coord_x_shift + vec2( 0.0, time * 0.75 ) );

	float inv_temperature=
		mix(
			texture( tex, tex_coord ).r,
			( 1.0 - gl_PointCoord.y ),
			0.4 );

	inv_temperature= 1.0 - ( 1.0 - inv_temperature ) * ( 0.7 + 0.3 * f_power );

	const float c_step_width= 0.05;
	const float c_step_end_pos= 0.62;
	float alpha= 1.0 - smoothstep( c_step_end_pos - c_step_width, c_step_end_pos, inv_temperature );
	alpha*= 1.0 - smoothstep( 0.8, 1.0, 1.0 - gl_PointCoord.y );

	if( alpha < 0.04 ) discard;

	vec4 color_for_temperature= texture( spectre, vec2( inv_temperature, 0.0 ) );

	color= vec4( color_for_temperature.rgb, alpha );
}
