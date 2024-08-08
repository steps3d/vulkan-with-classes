#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout ( set = 0, binding = 1 ) uniform sampler samp;
layout ( set = 0, binding = 2 ) uniform texture2D textures [];

layout(location = 0) in vec2 tex;
layout(location = 0) out vec4 color;

void main() 
{
	vec2  t  = tex * vec2 ( 4.0, 4.0 );
	ivec2 i  = ivec2 ( t );
	int   no = (i.x + 4 * i.y) % 8;
	
	color = texture ( sampler2D ( textures [nonuniformEXT ( no )], samp ), tex );
}