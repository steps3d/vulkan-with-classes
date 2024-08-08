#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 tex;
layout(location = 1) in vec3 n;
layout(location = 2) in vec3 l;

layout(location = 0) out vec4 color;

//uniform sampler2D	image;

layout(binding = 1) uniform sampler2D decalMap;

const float ka   = 0.2;
const float kd   = 0.8;

void main(void)
{
	vec3  n2   = normalize ( n );
	vec3  l2   = normalize ( l );
	float diff = max       ( dot ( n2, l2 ), 0.0 );
	vec4  clr  = texture   ( decalMap, tex );

	color = (ka + kd*diff) * clr;
}
