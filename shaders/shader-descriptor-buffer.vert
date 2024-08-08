#version 450

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec2 texCoord;
layout ( location = 2 ) in vec3 normal;

layout ( set = 0, binding = 0 ) uniform Camera 
{
	mat4	proj;
	mat4	view;
} camera;

layout ( set = 1, binding = 0 ) uniform Model 
{
	mat4	model;
} model;

layout ( location = 0 ) out vec2 tex;

void main() 
{
	gl_Position  = camera.proj * camera.view * model.model * vec4 ( pos, 1.0 );
	tex          = texCoord;
}