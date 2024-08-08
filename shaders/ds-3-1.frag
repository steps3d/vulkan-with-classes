#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D diffMap;
layout(binding = 2) uniform sampler2D bumpMap;

layout(location = 0) in vec2 tex;
layout(location = 1) in vec3 n;
layout(location = 2) in vec3 t;
layout(location = 3) in vec3 b;
layout(location = 4) in vec4 p;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 color2;

vec2 encodeNormal ( in vec3 normal )
{
	if ( normal.z < 0.0 )
		return vec2 ( normal.x + 3.0, normal.y );
		
	return normal.xy;
}

void main(void)
{
	const float specPower = 70.0;
	const float ks        = 0.7;
	
	vec4 c  = texture      ( diffMap, tex );
	vec3 nt = normalize    ( 2.0*texture ( bumpMap, tex ).rgb - vec3( 1.0 ) );
	vec2 nn = encodeNormal ( nt.x*t + nt.y*b + nt.z*n );
	
	color  = vec4 ( p.z, nn.x, nn.y, ks );
	color2 = vec4 ( c.rgb, specPower );
}
