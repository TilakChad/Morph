#version 330 core 
layout (location = 0) in vec2 aPos; 
layout (location = 1) in vec2 normal_data; 

uniform float thickness;

out vec4 g_normal; 

void main() {
    gl_Position = vec4(aPos,0.0f,1.0f);
    g_normal = vec4(normalize(vec2(normal_data.y, -normal_data.x)) * thickness, 0.0f, 0.0f);  
}
