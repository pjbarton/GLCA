#version 130

in vec4 vPosition;
in vec4 vColor;
out vec4 fragmentColor;

uniform mat4 utransform;


void main()
{
	//fragmentColor = vec4(1.0, 0.0, 0.0, 1.0);

	fragmentColor = vec4(vColor);
	gl_Position = utransform * vPosition;
}