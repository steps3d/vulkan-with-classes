#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	mat4 mvInv;
	vec4 light;
} ubo;

layout(binding = 1) uniform sampler2D normalMap;
layout(binding = 2) uniform sampler2D diffMap;

layout(location = 0) in vec2 tex;
layout(location = 1) in vec4 viewRay;

layout(location = 0) out vec4 color;

//in  vec4 pp;
//in  vec2 tex;
//in  vec4 viewRay;
//out vec4 color;

vec3 decodeNormal ( in vec2 nn )
{
	if ( nn.x > 1.5 )		// negative n.z
	{
		float	nx = nn.x - 3.0;
		float	nz = -sqrt( 1.0 - nx*nx - nn.y*nn.y );
		
		return vec3( nx, nn.y, nz );
	}
	
	return vec3 ( nn.x, nn.y, sqrt ( 1.0 - nn.x*nn.x - nn.y*nn.y ) );
}

void main(void)
{
	vec4	c1 = texture ( normalMap, tex );
	float	z  = c1.x;
	float	ks = c1.w;
	vec3	p  = viewRay.xyz * z / viewRay.z;
	vec3	n  = normalize(decodeNormal ( c1.yz ));
	vec4    c2 = texture( diffMap, tex );
	vec3	l  = normalize ( ubo.light.xyz - p );
	vec3	v  = normalize ( -p );
	vec3	h  = normalize ( l + v );
	float	diff = max ( 0.2, max( 0.0, dot ( l, n ) ) );	
	float	spec = pow ( max( 0.0, dot ( h, n ) ), c2.a );
	
	color = vec4 ( diff*c2.rgb + 0.3*vec3( spec ), 1.0 );
//color = c2;
}
