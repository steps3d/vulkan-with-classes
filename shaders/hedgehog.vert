#version 420 core

layout(std140, binding = 0) uniform UniformBufferObject 
{
	uniform mat4  mv;
	uniform mat4  nm;
	uniform mat4  proj;
	uniform	vec4  eye;		// eye position
	uniform vec4  lightDir;
	uniform int   detail;
	uniform float droop;
	uniform int   length;
	uniform float step;
} ubo;

layout(location = 0) in vec3 pos;
//layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;


layout(location = 0) out vec3 	norm;
layout(location = 1) out vec4	color;

void main(void)
{
	//vec4 	p = ubo.mv * vec4 ( pos, 1.0 );
	vec3	n = normalize ( mat3 ( ubo.nm ) * normal );
	vec3	l = normalize ( ubo.lightDir.xyz );
	
	gl_Position  = ubo.mv * vec4 ( pos, 1.0 );
	norm         = n;
	color        =  (0.2 + 0.8 * max ( 0.0, dot ( n, l ) ) ) * vec4 ( 0.7, 0.5, 0.1, 1.0 );
}
