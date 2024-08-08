#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 binormal;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 eye;		// eye position
	vec4 lightDir;
	mat4 nm;
} ubo;

layout(location = 0) out vec2 tx;
layout(location = 1) out vec3 v;
layout(location = 2) out vec3 l;
layout(location = 3) out vec3 h;

void main(void)
{
	vec4 p  = ubo.model * vec4 ( pos, 1.0 );
	mat3 nm = mat3 ( ubo.model );  	// XXX - should use mat3(ubo.nm)

	vec3	n  = normalize ( nm * normal     );
	vec3	t  = normalize ( nm * tangent    );
	vec3	b  = normalize ( nm * binormal   );
	vec3	l1 = normalize ( ubo.lightDir.xyz );
	vec3	v1 = normalize ( ubo.eye.xyz - p.xyz );
	vec3	h1 = normalize ( l1 + v1             );
	
				// convert to TBN
	v  = vec3 ( dot ( v1, t ), dot ( v1, b ), dot ( v1, n ) );
	l  = vec3 ( dot ( l1, t ), dot ( l1, b ), dot ( l1, n ) );
	h  = vec3 ( dot ( h1, t ), dot ( h1, b ), dot ( h1, n ) );
	tx = tex * vec2 ( 1, 3 );
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4 ( pos, 1.0 );
	//gl_Position  = ubo.proj * p;
}

