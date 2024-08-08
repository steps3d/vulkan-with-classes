#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec4 pos;
layout(location = 0) out vec2 tex;

void main(void)
{
	tex         = pos.zw;
	gl_Position = vec4 ( pos.x, pos.y, 0.0, 1.0 );
}

