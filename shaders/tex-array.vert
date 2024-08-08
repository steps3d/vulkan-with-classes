#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout ( std140, binding = 0 ) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 nm;
} ubo;

layout ( location = 0 ) in  vec3 pos;
layout ( location = 1 ) in  vec2 tex;
layout ( location = 0 ) out vec2 tx;

void main(void)
{
	gl_Position = ubo.proj * ubo.mv * vec4 ( pos, 1.0 );
	tx          = tex  * vec2 ( 1.0, 4.0 );	
}
