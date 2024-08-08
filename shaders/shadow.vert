#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 nm;
	vec4 light;
	mat4 shadowMat;
} ubo;

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec2 texCoord;
layout ( location = 2 ) in vec3 normal;

layout(location = 0) out vec2 tex;
layout(location = 1) out vec3 n;
layout(location = 2) out vec3 l;
layout(location = 3) out vec4 shadowPos;


void main(void)
{
	vec4 	p  = ubo.mv * vec4 ( pos, 1.0 );
	mat3	nm = mat3 ( ubo.nm );
	
	gl_Position  = ubo.proj * p;
	n            = normalize ( nm * normal );
	l            = normalize ( ubo.light.xyz - p.xyz );
	tex          = texCoord;
	shadowPos    = ubo.shadowMat * p;
//shadowPos = ubo.shadowMat * vec4 ( pos, 1.0 );
}
