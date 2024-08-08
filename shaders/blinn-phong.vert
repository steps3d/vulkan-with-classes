#version 450

layout(std140, binding = 0) uniform UniformBufferObject 
{
	uniform mat4  mv;
	uniform mat4  nm;
	uniform mat4  proj;
	uniform	vec4  eye;		// eye position
	uniform vec4  lightDir;
//	uniform int   detail;
//	uniform float droop;
//	uniform int   len;
//	uniform float step;
}ubo;

///uniform mat4 proj;
//uniform mat4 mv;
//uniform mat3 nm;
//uniform	vec3 eye;		// eye position
//uniform vec3 lightDir;

layout ( location = 0 ) in vec3 pos;
layout ( location = 2 ) in vec3 normal;

layout ( location = 0 ) out vec3 n;
layout ( location = 1 ) out vec3 v;
layout ( location = 2 ) out vec3 l;
layout ( location = 3 ) out vec3 h;

void main(void)
{
	vec4 	p = ubo.mv * vec4 ( pos, 1.0 );
	
	gl_Position  = ubo.proj * p;
	n            = normalize ( mat3 ( ubo.nm ) * normal );
	v            = normalize ( ubo.eye.xyz - p.xyz );					// vector to the eye
	l            = normalize ( ubo.lightDir.xyz );
	h            = normalize ( v + l );
}
