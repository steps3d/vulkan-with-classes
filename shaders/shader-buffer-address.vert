#version 450

#extension	GL_EXT_buffer_reference : require

layout(binding = 0) uniform UniformBufferObject 
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

struct	Instance
{
	vec4	offs;
	vec4	color;
};

layout ( buffer_reference, std430, buffer_reference_align=16) buffer BufferPtr
{
	Instance instances [];
};

layout( push_constant ) uniform constants
{
	BufferPtr	ptr;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

void main() 
{
	gl_Position  = ubo.proj * ubo.view * ubo.model * vec4 ( 0.3 * inPosition + push.ptr.instances [gl_InstanceIndex].offs.xyz, 1.0 );
	fragTexCoord = inTexCoord;
	fragColor    = push.ptr.instances [gl_InstanceIndex].color;
}
