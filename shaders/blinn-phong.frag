#version 450

layout ( location = 0 ) in vec3 n;
layout ( location = 1 ) in vec3 v;
layout ( location = 2 ) in vec3 l;
layout ( location = 3 ) in vec3 h;

layout ( location = 0 ) out vec4 color;

//layout(binding = 1) uniform sampler2D albedoMap;

const float specPower = 70.0;
const vec4  clr  = vec4 ( 0.7, 0.1, 0.1, 1.0 );

void main(void)
{
	vec3  n2   = normalize ( n );
	vec3  l2   = normalize ( l );
	vec3  h2   = normalize ( h );
	float nl   = dot       ( n2, l2 );
	float diff = max       ( nl, 0.0 );
	float spec = pow       ( max ( dot ( n2, h2 ), 0.0 ), specPower );
	float ka   = 0.2;
	float kd   = 0.8;
	float ks   = 0.5;

	color = (ka + kd*diff)*clr + ks*vec4(spec);
}
