uniform float particle_size= 256.0;

uniform mat4 mat;

in vec3 pos;
in vec2 tex_coord;
in float power;

out vec2 f_tex_coord;
out float f_power;

void main()
{
	f_tex_coord= tex_coord;
	f_power= power;

	vec4 world_position= vec4( pos, 1.0 );

	gl_Position= mat * world_position;
	gl_PointSize= max( particle_size / gl_Position.w, 0.5 );
}
