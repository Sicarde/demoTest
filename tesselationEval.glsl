#version 400 core

layout(triangles) in;
uniform mat4 mvp;
out int geometryGenerateSomething;

void main()
{
	vec3 p0 = gl_TessCoord.x * gl_in[0].gl_Position.xyz;
	vec3 p1 = gl_TessCoord.y * gl_in[1].gl_Position.xyz;
	vec3 p2 = gl_TessCoord.z * gl_in[2].gl_Position.xyz;
	gl_Position = mvp * vec4(p0 + p1 + p2, 1);
	geometryGenerateSomething = 0;
	//gl_tessCoord is the location within the tessellated abstract patch for this particular vertex
	if (gl_TessCoord.x == 0.0 && gl_TessCoord.y == 0.0/* && gl_PrimitiveID == 1*/) {
	    geometryGenerateSomething = 1;
	}
}

//gl_tessCoord(primitive0 with tessLevelInner1&tessLevelOuter2)
//0,0 0.5,0 1,0
//    0,0.5 1,1
//	    0,1

//gl_tessCoord(primitive1 with tessLevelInner1&tessLevelOuter2)
//0,0
//0,0.5 0.5,0
//0,1   0.5,0.5 1,0
