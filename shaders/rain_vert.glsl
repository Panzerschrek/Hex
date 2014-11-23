#version 330

uniform float particle_size= 64.0;

uniform float time;
uniform float rain_velocity= 0.0;
uniform vec3 particle_zone_size;
uniform vec3 particle_zone_coord;

uniform mat4 mat;

in vec4 coord;

void main()
{
	vec3 particle_coord= coord.xyz;
	particle_coord.z-= rain_velocity * time;
	particle_coord= mod( particle_coord - particle_zone_coord, particle_zone_size ) + particle_zone_coord;

	gl_Position= mat * vec4( particle_coord.xyz, 1.0 );
	gl_PointSize= max( particle_size / gl_Position.w, 1.0 );
}