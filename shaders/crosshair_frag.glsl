out vec4 color;

uniform sampler2D tex;

noperspective in vec2 f_tex_coord;

void main()
{
	color= texture( tex, f_tex_coord ) * 0.5;
}
