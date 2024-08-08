#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( binding = 1 ) uniform sampler2D image;

layout(location = 0) in  vec2 tex;
layout(location = 0) out vec4 color;

void main(void)
{
	color = vec4 ( texture ( image, tex ).r );
}
