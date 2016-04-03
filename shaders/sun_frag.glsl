uniform sampler2D tex;
out vec4 color;
void main()
{
	color= texture( tex, gl_PointCoord );
	gl_FragDepth= 1.0;
}
