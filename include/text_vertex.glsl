#version 330 core 

layout (location = 0) in vec2 aPos; 
layout (location = 1) in vec2 Tex;
// might need a matrix somewhere here 

out vec2 TexCoord; 
uniform mat4 scene; 

void main()
{
	gl_Position = scene * vec4(aPos,0.0f,1.0f);
	TexCoord = Tex; 
}