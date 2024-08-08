// From gamedev.ru/forum !!!
//
// vec3 F0 = abs((1 - IOR)/(1 + IOR));
// F0 *= F0;
// F0 = mix(F0, diffuse_coeff.rgb, metalness);
// diffuse.rgb = mix(diffuse_coeff.rgb, vec3(0.0), metalness);
// vec3 fresnel = F0 + (vec3(1.0) - F0) * pow(1.0 - clamp(dot(V, N), 0.0, 1.0), 5.0);
// specular.rgb = fresnel;
// diffuse.rgb *= vec3(1.0)-fresnel;
//

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec4 eye;		// eye position
	vec4 lightDir;
	mat3 nm;
} ubo;

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D metallnessMap;
layout(binding = 3) uniform sampler2D normalMap;
layout(binding = 4) uniform sampler2D roughnessMap;

layout(location = 0) in  vec2 tx;
layout(location = 1) in  vec3 v;
layout(location = 2) in  vec3 l;
layout(location = 3) in  vec3 h;
layout(location = 0) out vec4 color;

const vec3      lightColor = vec3 ( 1.0 );

const float gamma = 2.2;
const float pi    = 3.1415926;
const float FDiel = 0.04;		// Fresnel for dielectrics

vec3 fresnel ( in vec3 f0, in float product )
{
    return mix ( f0, vec3 (1.0), pow(1.0 - product, 5.0) );
}

float D_beckmann ( in float roughness, in float NdH )
{
    float m    = roughness * roughness;
    float m2   = m * m;
    float NdH2 = NdH * NdH;
	
    return exp( (NdH2 - 1.0) / (m2 * NdH2) ) / (pi * m2 * NdH2 * NdH2);
}

float D_GGX ( in float roughness, in float NdH )
{
    float m  = roughness * roughness;
    float m2 = m * m;
	float ndh2 = NdH * NdH;
    //float d  = (NdH * m2 - NdH) * NdH + 1.0;
	float d  = ndh2 * (m2 - 1.0) + 1.0;
	
    return m2 / (pi * d * d);
}

float G_schlick ( in float roughness, in float nv, in float nl )
{
roughness = (roughness+1)*(roughness+1)/8;

    float k = roughness * roughness * 0.5;
    float V = nv * (1.0 - k) + k;
    float L = nl * (1.0 - k) + k;
	
//    return 0.25 / (V * L);
	return nv * nl / ( V * L );
}

//////////
// Schick from UE4
// k =  sqr(roughness+1) / 8
// G1(v) = nv /  (nv*(1-k)+k
// G = G1(v)*G1(l)


// Schlick from graphicrants.blogspot.com
// G(v) = nv / (nv*(k-1) + k)
// k = roughness * sqrt ( 2/pi)


// Cook-Torrange
// G= min(1, 2*nh*nv/vh, 2(nh*nl/vh )



float G_neumann ( in float nl, in float nv )
{
	return nl * nv / max ( nl, nv );
}

float G_klemen ( in float nl, in float nv, in float vh )
{
	return nl * nv / (vh * vh );
}

float G_default ( in float nl, in float nh, in float nv, in float vh )
{
	return min ( 1.0, min ( 2.0*nh*nv/vh, 2.0*nh*nl/vh ) );
}

vec3 cookTorrance ( in float nl, in float nv, in float nh, in vec3 f0, in float roughness )
{
//    float D = D_blinn(roughness, NdH);
//    float D = D_beckmann(roughness, NdH);

    float D = D_GGX     ( roughness, nh );
    float G = G_schlick ( roughness, nv, nl );

	return f0 * D * G;
}

void main ()
{
	vec3 base        = texture ( albedoMap,     tx ).xyz;
	vec3 n           = texture ( normalMap,     tx ).xyz * 2.0 - vec3 ( 1.0 );
	float roughness  = texture ( roughnessMap,  tx ).x;
	float metallness = texture ( metallnessMap, tx ).x;

n= vec3 ( 0, 0, 1 );

	base = pow ( base, vec3 ( gamma ) );
	
	vec3  n2   = normalize ( n );
	vec3  l2   = normalize ( l );
	vec3  h2   = normalize ( h );
	vec3  v2   = normalize ( v ); 
	float nv   = max ( 0.0, dot ( n2, v2 ));
	float nl   = max ( 0.0, dot ( n2, l2 ));
	float nh   = max ( 0.0, dot ( n2, h2 ));
	float hl   = max ( 0.0, dot ( h2, l2 ));
	float hv   = max ( 0.0, dot ( h2, v2 ));

	vec3 F0          = mix ( vec3(FDiel), base, metallness );
	vec3 specfresnel = fresnel ( F0, nv );//hv );
	vec3 spec        = cookTorrance ( nl, nv, nh, specfresnel, roughness ) / ( 0.001 + 4.0 * nl * nv );
	vec3 diff        = (vec3(1.0) - specfresnel)  / pi;
	
	color = pow ( vec4 ( ( diff * mix ( base, vec3(0.0), metallness) + spec ) * lightColor, 1.0 ), vec4 ( 1.0 / gamma ) );

//color = vec4 ( G_schlick ( roughness, nv, nl ) );		// must be close to 0 when nv close to 0
//	float x    = 2.0 * nh / hv;
//	float g    = min(1.0, min (x * nl, x * nv));		// this is close to  - when nv is close to 0
//color = vec4 ( g );
/*
	const vec4  r0   = vec4 ( 1.0, 0.92, 0.23, 1.0);
	//const vec4  clr  = vec4 ( 1.0 );	//0.7, 0.1, 0.1, 1.0 );

					// compute Beckman
	float r2   = roughness * roughness;
	float nh2  = nh * nh;
	float ex   = -(1.0 - nh2)/(nh2 * r2);
	float d    = pow(2.7182818284, ex ) / (r2*nh2*nh2); 
	
	vec4  f    = mix(vec4(pow(1.0 - nv, 5.0)), vec4(1.0), r0);
	
					// default G
	float x    = 2.0 * nh / dot(v2, h);
	float g    = min(1.0, min (x * nl, x * nv));
	
					// resulting color
	vec4  ct   = f*(0.25 * d * g / nv);
	float diff = max(nl, 0.0);
	float ks   = 0.5;

	color = diff * vec4 (base,1.0) + ks * ct;
*/
}
