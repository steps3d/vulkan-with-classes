#version 450

//#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject 
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 tex;

void main() 
{
	gl_Position  = ubo.proj * ubo.view * ubo.model * vec4 ( inPosition, 1.0 );
	tex          = inTexCoord;
}