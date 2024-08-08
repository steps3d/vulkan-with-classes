#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 pos;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
//	mat4 mvInv;
	mat4 nm;
	vec4 light;
};

layout(location = 0) out vec2 tex;
layout(location = 1) out vec4 viewRay;

void main(void)
{
	vec4 base = vec4 ( pos.x / proj [0][0], pos.y / proj [1][1], -1.0, 1.0 );
	
	tex         = pos.zw;
	viewRay     = base;
	gl_Position = proj * viewRay;
}
