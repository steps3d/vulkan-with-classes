#version 450

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4	mv;
	mat4	proj;
	int	inner;
	int	outer;
} ubo;

layout ( vertices = 16 ) out;

void main(void)
{
	gl_TessLevelInner [0] = ubo.inner;
	gl_TessLevelInner [1] = ubo.inner;
	gl_TessLevelOuter [0] = ubo.outer;
	gl_TessLevelOuter [1] = ubo.outer;
	gl_TessLevelOuter [2] = ubo.outer;
	gl_TessLevelOuter [3] = ubo.outer;

	gl_out [gl_InvocationID].gl_Position = gl_in [gl_InvocationID].gl_Position;
}

