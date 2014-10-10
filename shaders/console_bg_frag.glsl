#version 330
out vec4 color;
uniform sampler2D tex;
uniform vec3 screen_size;
noperspective in vec2 tex_coord;
void main()
{
	color= texture( tex, tex_coord ) * step( 0.02, tex_coord.y );
}