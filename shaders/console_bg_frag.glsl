out vec4 color;
uniform sampler2D tex;
uniform vec3 screen_size;
noperspective in vec2 f_tex_coord;

void main()
{
	color= texture( tex, f_tex_coord );
}
