
uniform float particle_size= 64.0;

uniform vec3 particle_coord_delta = vec3(0.0, 0.0, 0.0);
uniform vec3 particle_zone_size;
uniform vec3 particle_zone_coord;

uniform mat4 mat;

in vec4 coord;

void main()
{
	vec3 particle_coord= coord.xyz * particle_zone_size; // convert coordinates in normalized short format in range [0;1] to range of particle zone size.
	particle_coord+= particle_coord_delta;
	particle_coord= mod( particle_coord - particle_zone_coord, particle_zone_size ) + particle_zone_coord;

	gl_Position= mat * vec4( particle_coord.xyz, 1.0 );
	gl_PointSize= max( particle_size / gl_Position.w, 0.5 );
}
