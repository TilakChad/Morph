#version 330 core 
uniform vec3 inColor; 
out vec4 color; 

in vec2 i_normal; 

void main() {
	// implement smooth step rendering 
	// color = vec4(0.7f,0.5f,0.7f,1.0f);
	color = mix(vec4(inColor,0.0f),vec4(inColor,1.0f),smoothstep(0,1,exp(-length(i_normal))));
}