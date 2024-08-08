#version 420 core

layout ( triangles ) in;
layout ( line_strip, max_vertices = 128 ) out;

layout(std140, binding = 0) uniform UniformBufferObject 
{
	uniform mat4  mv;
	uniform mat4  nm;
	uniform mat4  proj;
	uniform	vec4  eye;		// eye position
	uniform vec4  lightDir;
	uniform int   detail;
	uniform float droop;
	uniform int   len;
	uniform float step;
} ubo;

layout(location = 0) in  vec3  norm  [];
layout(location = 1) in  vec4  color [];
layout(location = 0) out vec4  clr;

void produceVertices ( in vec4 v, in vec3 n )
{
	for ( int i = 0; i <= ubo.len; i++ )
	{
		gl_Position = ubo.proj * v;
		clr         = color [0];	//vec4 ( 0, 1, 0, 1 );	//color [0];
		EmitVertex ();

		v.xyz += ubo.step * n;
		v.z   += ubo.droop * float ( i * i );
	}

	EndPrimitive ();
}

void main ()
{
	vec4 v0  = gl_in [0].gl_Position;
	vec4 v01 = gl_in [1].gl_Position - gl_in [0].gl_Position;
	vec4 v02 = gl_in [2].gl_Position - gl_in [0].gl_Position;
	vec3 n0  = normalize ( norm [0] );
	vec3 n01 = norm [1] - norm [0];
	vec3 n02 = norm [2] - norm [0];

	if ( dot ( n0, n01 ) < 0 )
		n01 = -n01;	

	if ( dot ( n0, n02 ) < 0 )
		n02 = -n02;	

	int	numLayers = 1 << ubo.detail;
	float	dt        = 1.0 / float ( numLayers );
	float	t         = 1.0;

	for ( int it = 0; it <= numLayers; it++ )
	{
		float smax = 1.0 - t;
		float ds   = smax / float ( it + 1 );

		for ( float s = 0; s < smax; s += ds )
		{
			vec4 v = v0 + s*v01 + t*v02;
			vec3 n = normalize ( n0 + s*n01 + t*n02 );

			produceVertices ( v, n );
		}

		t -= dt;
	}
}
