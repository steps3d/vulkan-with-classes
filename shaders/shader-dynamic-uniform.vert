#version 450

layout(binding = 0) uniform UniformBufferObject 
{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 offs;
	vec4 color;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() 
{
	gl_Position  = ubo.proj * ubo.view * ubo.model * vec4 ( 0.3 * inPosition + ubo.offs.xyz, 1.0 );
	fragTexCoord = inTexCoord;
	fragColor    = ubo.color;
}
