#version 450

#extension GL_EXT_descriptor_heap: require

layout ( set = 0, binding = 2 ) uniform texture2D image [3];
layout ( set = 0, binding = 3 ) uniform sampler textureSampler;

layout ( location = 0 ) in vec2 tex;
layout ( location = 0 ) out vec4 color;

layout ( push_constant ) uniform PushConsts 
{
	int	modelIndex;
	int	frameIndex;
} pushConsts;


void main() 
{
	color = texture ( sampler2D ( image [pushConsts.modelIndex], textureSampler ), tex * vec2 ( 1, 6 ) );
}