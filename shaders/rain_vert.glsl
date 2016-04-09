
uniform float particle_size;

uniform vec3 particle_coord_delta = vec3(0.0, 0.0, 0.0);
uniform vec3 particle_zone_size;
uniform vec3 particle_zone_coord;

uniform sampler2DShadow heightmap;
uniform mat4 heightmap_matrix;

uniform mat4 mat;

in vec4 coord;

void main()
{
	vec3 particle_coord= coord.xyz * particle_zone_size; // convert coordinates in normalized short format in range [0;1] to range of particle zone size.
	particle_coord+= particle_coord_delta;
	particle_coord= mod( particle_coord - particle_zone_coord, particle_zone_size ) + particle_zone_coord;

	vec4 world_position= vec4( particle_coord.xyz, 1.0 );

	vec3 heightmap_pos= ( heightmap_matrix * world_position ).xyz;
	heightmap_pos= heightmap_pos * 0.5 + vec3( 0.5, 0.5, 0.5 );
	float in_shadow= texture( heightmap, heightmap_pos );
	world_position.z-= step( in_shadow, 0.5 ) * 256.0;

	gl_Position= mat * world_position;
	gl_PointSize= max( particle_size / gl_Position.w, 0.5 );
}
