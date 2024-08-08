#version 450

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() 
{
	int	ix   = gl_InstanceIndex  % 8;
	int	iy   = (gl_InstanceIndex  / 8) % 8;
	int	iz   = gl_InstanceIndex / 64;
	vec3	offs = vec3 ( ix * 2 - 8, iy * 2 - 8, iz * 2 - 8 );

	gl_Position  = ubo.proj * ubo.view * ubo.model * vec4 ( inPosition + offs, 1.0 );
	fragTexCoord = inTexCoord;
}
