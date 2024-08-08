#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 tex;
layout(location = 1) in vec3 n;
layout(location = 2) in vec3 l;
layout(location = 3) in vec4 shadowPos;
layout(location = 0) out vec4 color;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 nm;
	vec4 light;
	mat4 shadowMat;
} ubo;

layout ( binding = 1 ) uniform sampler2D image;
layout ( binding = 2 ) uniform sampler2D shadowMap;

const float bias = -0.0001;

void main(void)
{
	vec3  n2   = normalize ( n );
	vec3  l2   = normalize ( l );
	float diff = max ( dot ( n2, l2 ), 0.0 );
	vec4  clr  = texture ( image, tex );
	float ka   = 0.2;
	float kd   = 0.8;

	vec3 p = shadowPos.xyz / shadowPos.w;
	
	p.xy = p.xy * 0.5 + vec2 ( 0.5 );		// remap to [0,1]
	
	float	closestDepth = texture ( shadowMap, p.xy ).r;
	float	currentDepth = p.z;
	float	shadow       = currentDepth + bias > closestDepth ? 1.0 : 0.0;
	
	color = ka*clr + kd*diff * clr * (1.0 - shadow);
// color = vec4 ( texture ( shadowMap, p.xy ).r < 1.0 );
}
