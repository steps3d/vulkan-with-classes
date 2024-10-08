#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 binormal;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	mat4 mv;
	mat4 proj;
	vec4 eye;		// eye position
	vec4 lightDir;
} ubo;

layout( push_constant ) uniform constants
{
	mat4	matrix;
	int	albedo, metallic, normal, roughness;
} push;

layout(location = 0) out vec2 tx;
layout(location = 1) out vec3 v;
layout(location = 2) out vec3 l;
layout(location = 3) out vec3 h;

void main(void)
{
	mat4	mv = ubo.mv * push.matrix;
	mat3	nm = inverse ( transpose ( mat3 ( mv ) ) );
	vec4	p  = mv * vec4 ( pos, 1.0 );

	vec3	n  = nm * normal; //normalize ( nm * normal     );
	vec3	t  = nm * tangent; //normalize ( nm * tangent    );
	vec3	b  = nm * binormal; //normalize ( nm * binormal   );
	vec3	l1 = normalize ( ubo.lightDir.xyz );
	vec3	v1 = normalize ( ubo.eye.xyz - p.xyz );
	vec3	h1 = normalize ( l1 + v1             );
	
				// convert to TBN
	v  = vec3 ( dot ( v1, t ), dot ( v1, b ), dot ( v1, n ) );
	l  = vec3 ( dot ( l1, t ), dot ( l1, b ), dot ( l1, n ) );
	h  = vec3 ( dot ( h1, t ), dot ( h1, b ), dot ( h1, n ) );
	tx = tex;
	gl_Position = ubo.proj * mv * vec4 ( pos, 1.0 );
}

