uniform float particle_size= 256.0;

uniform mat4 mat;

in vec3 pos;
in float power;

out vec2 f_tex_coord_x_shift;
out float f_power;

void main()
{
	f_tex_coord_x_shift= vec2( pos.x + pos.y, pos.z );
	f_power= power;

	vec4 world_position= vec4( pos, 1.0 );

	gl_Position= mat * world_position;
	gl_PointSize= max( particle_size / gl_Position.w, 0.5 );
}
