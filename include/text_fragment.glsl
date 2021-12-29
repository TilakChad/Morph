#version 330 core

out vec4 color_vec; 

in vec2 TexCoord; 
uniform sampler2D font;


void main()
{
	vec4 color = texture(font,TexCoord);
	color_vec = vec4(0.0f,0.0f,0.0f,color.r);
}