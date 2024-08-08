#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 nm;
	vec4 light;
} ubo;

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec2 texCoord;
layout ( location = 2 ) in vec3 normal;
layout ( location = 3 ) in vec3 tangent;
layout ( location = 4 ) in vec3 binormal;

layout(location = 0) out vec2 tex;
layout(location = 1) out vec3 n;
layout(location = 2) out vec3 t;
layout(location = 3) out vec3 b;
layout(location = 4) out vec4 p;

void main(void)
{
	mat3 nm = mat3 ( ubo.nm );			// ubo.mv );

	tex         = 0.25*texCoord;
	n           = nm * normal;
	t           = nm * tangent;
	b           = nm * binormal;
	p           = ubo.mv * vec4 ( pos, 1.0 );
	gl_Position = ubo.proj * p;
}
