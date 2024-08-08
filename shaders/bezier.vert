//
// Bezier qubic patch
//

#version 450

#extension GL_GOOGLE_include_directive : require

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4	mv;
	mat4	proj;
	int	inner;
	int	outer;
} ubo;

layout(location = 0) in vec3 pos;

void main(void)
{
	gl_Position = ubo.proj * ubo.mv * vec4 ( pos, 1.0 );
}
