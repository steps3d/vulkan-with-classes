#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
//	mat4 mvInv;
	mat4 nm;
	vec4 light;
};

layout(binding = 1) uniform sampler2D diffMap;

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
	vec4    c2 = texture( diffMap, tex );

	color = vec4 ( 0, 0.2, 0, 1 ) + c2;

//	if ( c2.r > 0.0 )
//	color = vec4  ( 1 );
}
