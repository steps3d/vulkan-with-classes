#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in  vec2 tx;
layout ( location = 0 ) out vec4 color;

layout ( set = 0, binding = 1 ) uniform sampler samp;
layout ( set = 0, binding = 2 ) uniform texture2D textures [18];

void main(void)
{
	vec2  t  = tx * vec2 ( 4.0, 4.0 );
	ivec2 i  = ivec2 ( t );
	int   no = (i.x + 4 * i.y) % 8;
	
	color = texture ( sampler2D ( textures [no], samp ), t );
}
