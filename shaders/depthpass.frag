#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec2 tex;
//layout(location = 1) in vec3 n;
//layout(location = 2) in vec3 l;

//layout(location = 0) out vec4 color;

//uniform sampler2D	image;

//layout(binding = 1) uniform sampler2D decalMap;

//const float ka   = 0.2;
//const float kd   = 0.8;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 mvInv;
	vec4 light;
	mat4 shadowMat;
} ubo;

layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D diffMap;

void main(void)
{
    //color = vec4 ( 1, 1, 0, 1 );
}
