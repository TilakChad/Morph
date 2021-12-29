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
	int delX = abs(int(scr.x-center.x)); 
	int delY = abs(int(scr.y-center.y));
	
	int X = int(scale.x); 
	if (X % 2 != 0) 
		X = X + 1; 

	int Y = X;

	int halfX = X / 2; 
	int halfY = halfX; 

	// TODO :: Rewrite it in branchless way 
	if ( (delX % halfX <= grid_width) || (delY % halfY <= grid_width))
		color = vec4(0.0f,0.7f,0.7f,1.0f); 
	else
		color = vec4(1.0f,1.0f,1.0f,1.0f);
		
	if ( (delX % X <= grid_width+2) || (delY % Y <= grid_width+2))
		color = vec4(0.5f,0.5f,0.5f,1.0f);


	if (abs(scr.x - center.x) < 3.0f) 
		color = vec4(1.0f,0.0f,0.0f,1.0f);
	if (abs(scr.y - center.y) < 3.0f) 
		color = vec4(1.0f,0.0f,0.0f,1.0f);
}