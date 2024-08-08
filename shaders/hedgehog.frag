#version 420 core

layout(location = 0) in  vec4 clr;
layout(location = 0) out vec4 color;

void main(void)
{
	color = clr;
}
