#version 330 core 

uniform vec3 inColor; 
out vec4 color; 
void main()
{
	// color = vec4(0.0f,1.0f,0.0f,1.0f);
	color = vec4(inColor,1.0f);
}