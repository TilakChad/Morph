#version 450 core 

layout (lines) in; 
layout (triangle_strip, max_vertices = 12) out; 
// They should not be rendered 

in vec4 g_normal[]; 

uniform mat4 scene;
uniform mat4 transform; 

out vec2 i_normal; 

float determinant(vec2 a, vec2 b) 
{
	return a.x * b.y - a.y * b.x; 
}

void main() 
{
	vec2 cvec = vec2(g_normal[0]), dvec = vec2(g_normal[1]); 

	if (determinant(cvec,dvec) < 0) 
	{
		cvec = cvec * -1.0f; 
		dvec = dvec * -1.0f; 
	}
	float dot_val = dot(cvec,vec2(dvec.y, -dvec.x));
	float t = 0; 
	if (dot_val != 0) 
		t = (dot(cvec,cvec) - dot(cvec,dvec)) / dot_val; 

	vec2 mvec = dvec + vec2(dvec.y,-dvec.x) * t; 

	gl_Position = scene * (transform * gl_in[0].gl_Position + vec4(cvec,0.0f,0.0f)); 
	i_normal = normalize(cvec); 
	EmitVertex(); 

	gl_Position = scene * (transform * gl_in[0].gl_Position - vec4(cvec,0.0f,0.0f)); 
	i_normal = -normalize(cvec); 
	EmitVertex(); 

	gl_Position = scene * (transform * gl_in[1].gl_Position + vec4(cvec,0.0f,0.0f)); 
	i_normal = normalize(cvec); 
	EmitVertex(); 

	gl_Position = scene * (transform * gl_in[1].gl_Position - vec4(cvec,0.0f,0.0f)); 
	i_normal = -normalize(cvec); 
	EmitVertex(); 
	EndPrimitive(); 

	// Emit another primitive set
	gl_Position = scene * (transform * gl_in[1].gl_Position + vec4(cvec,0.0f,0.0f)); 
	i_normal = normalize(cvec);
	EmitVertex(); 

	gl_Position = scene * transform * gl_in[1].gl_Position; 
	i_normal = vec2(0.0f,0.0f);
	EmitVertex(); 

	gl_Position = scene * (transform * gl_in[1].gl_Position + vec4(mvec,0.0f,0.0f)); 
	i_normal = normalize(mvec);
	EmitVertex(); 

	gl_Position = scene * (transform * gl_in[1].gl_Position + vec4(dvec,0.0f,0.0f)); 
	i_normal = normalize(dvec);
	EmitVertex(); 
	EndPrimitive(); 
}