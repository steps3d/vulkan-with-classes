#version 450

layout ( set = 2, binding = 0 ) uniform sampler2D image;

layout ( location = 0 ) in vec2 tex;
layout ( location = 0 ) out vec4 color;

void main() 
{
	color = texture ( image, tex );
}