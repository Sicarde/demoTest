#version 400 core

layout(triangles) in;
//out vec3 tePatchDistance;
uniform mat4 mvp;

void main()
{
	vec3 p0 = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_TessCoord.y * gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_TessCoord.z * gl_in[2].gl_Position.xyz;
	//tePatchDistance = gl_TessCoord;
	gl_Position = mvp * vec4(p0 + p1 + p2, 1);
}
