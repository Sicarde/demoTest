#version 400 core

layout(vertices = 3) out;
//uniform float TessLevelInner;
//uniform float TessLevelOuter;

void main()
{
	float TessLevelInner = 1; //1 is no tesselation done; 2 is one point, 3 is a triangle, etc.
	float TessLevelOuter = 2;
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	if (gl_InvocationID == 0) {
		//if unclear, check images on internet such as https://i.stack.imgur.com/p68tW.png
		//or the explanations here: http://www.informit.com/articles/article.aspx?p=2120983
		gl_TessLevelInner[0] = TessLevelInner;//inner is the tesselation that will appear at the center (the higher the value the more it will be split)
		gl_TessLevelOuter[0] = TessLevelOuter;//outer 0 1 and 2 are the outside of the triangle, the higher the value the more the edges will be split
		gl_TessLevelOuter[1] = TessLevelOuter;
		gl_TessLevelOuter[2] = TessLevelOuter;
	}
}
