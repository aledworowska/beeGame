#version 330 core

layout( location = 0 ) in vec4 inPosition;

uniform mat4 matPVM;
out vec4 fragPos;

void main()
{
	gl_Position = matPVM * inPosition;
	fragPos = inPosition;
}
