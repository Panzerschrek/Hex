uniform float brightness;

out vec4 color;

in vec2 f_brightness_spectre;

void main()
{
	const vec3 brown_dwarf= vec3( 0.5, 0.3, 0.2 );
	const vec3 blue_giant= vec3( 0.8, 0.9, 1.3 );

	color= vec4( mix( brown_dwarf, blue_giant, f_brightness_spectre.y ), f_brightness_spectre.x * brightness );
	gl_FragDepth= 1.0;
}
