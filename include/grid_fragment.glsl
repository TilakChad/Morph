#version 330 core 

out vec4 color; 

// not working with uniform buffer for now 

// Lets try drawing a checkerboard 
// uniform int scale;

uniform vec2 scale;
uniform int grid_width;

uniform vec2 center; 

void main() 
{
	vec2 scr = gl_FragCoord.xy;

	if ( (int(abs(scr.x - center.x)) % int(scale.x/2.0f) <= grid_width) || (int(abs(scr.y - center.y)) % int(scale.y/2.0f) <= grid_width))
	{
		color = vec4(0.0f,0.7f,0.7f,1.0f); 
	}
	else
		color = vec4(1.0f,1.0f,1.0f,1.0f);
		// color = vec4(0.1f,0.1f,0.1f,1.0f);
		

	if ( (int(abs(scr.x - center.x)) % int(scale.x) <= grid_width+2) || (int(abs(scr.y - center.y)) % int(scale.y) <= grid_width+2))
		// color = vec4(1.0f,0.0f,1.0f,1.0f); 
		color = vec4(0.1f,0.1f,0.1f,1.0f);
	// else
	//	color = vec4(0.1f,0.1f,0.1f,1.0f);

	if (abs(scr.x - center.x) < 3.0f) 
		color = vec4(1.0f,0.0f,0.0f,1.0f);
	if (abs(scr.y - center.y) < 3.0f) 
		color = vec4(1.0f,0.0f,0.0f,1.0f);
}