#version 330 core 

out vec4 color; 

in vec3 fcolor; 

void main() 
{
	color = vec4(fcolor,1.0f); 
}