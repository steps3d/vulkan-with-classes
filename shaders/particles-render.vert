#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec4 pos;

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main()
{
	gl_PointSize = 1.0;
	gl_Position  = ubo.proj * ubo.view * ubo.model * vec4 ( pos.xyz, 1.0 );
}
