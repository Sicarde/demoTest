#version 400 core
#define M_PI 3.1415926535897932384626433832795

layout(triangles) in;
layout(triangle_strip, max_vertices = 240) out; //too much but enough for whatever

out vec3 position;
in int geometryGenerateSomething[3];

uniform float u_time;
uniform mat4 rotation;

vec3 GetNormal(vec3 vertex1, vec3 vertex2, vec3 vertex3) { //currently not used nor tested
	vec3 a = vertex1 - vertex2;// vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
	vec3 b = vertex3 - vertex2;// vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
	return normalize(cross(a, b));
}

void emitTriangle(vec4 v1, vec4 v2, vec4 v3) {
    gl_Position = v1 * rotation;
    position = gl_Position.xyz;
    EmitVertex();
    gl_Position = v2 * rotation;
    position = gl_Position.xyz;
    EmitVertex();
    gl_Position = v3 * rotation;
    position = gl_Position.xyz;
    EmitVertex();
    EndPrimitive();
}

void emitQuad(vec4 v1, vec4 v2, vec4 v3, vec4 v4) {
    emitTriangle(v1, v2, v3);
    emitTriangle(v2, v3, v4);
}

vec4 ampersandCurve(vec2 p) { //cf wikipedia
    //(y2 - x2)(x-1)(2x-3)=4(x2+y2-2x)2
    float color = clamp((pow(p.y, 2.0) - pow(p.x, 2.0)) * (p.x - 1.0) * (2.0 * p.x - 3.0) - 4.0 * pow(pow(p.x, 2.0) + pow(p.y, 2.0) - 2.0 * p.x, 2.0), 0.0, 1.0);
    return vec4(p.x, color, 0.0, 1.0);
}

//sidesNumber is the number of sides for the generated shape (ex 3 == triangle, 4 == rectangle, etc.)
//extrudeVector is the vector added to the generated vertex when extruding (for example you can put the normal here or a z value of 1 or whatever)
//originalPoint is the basic vertex
//scale is how far from the vertex your shape will be (think of it as the width and height of a rectangle if you have 4 sides)
//if join == true -> it will extrude a point only (ex: pyramid), else it will extrude a face
void emitShapeExtrude(int sidesNumber, vec4 extrudeVector, vec4 originalPoint, vec2 scale, bool join) {
	for (int i = 0; i < sidesNumber; ++i) {
		// Angle between each side in radians
		float ang1 = M_PI * 2.0 / sidesNumber * i;
		float ang2 = M_PI * 2.0 / sidesNumber * (i + 1);

		// Offset from center of point
		vec4 offset1 = vec4(cos(ang1) * scale.x, -sin(ang1) * scale.y, 0.0, 0.0);
		vec4 offset2 = vec4(cos(ang2) * scale.x, -sin(ang2) * scale.y, 0.0, 0.0);
		vec4 v1 = originalPoint + offset1;
		vec4 v2 = originalPoint + offset2;
		vec4 v3 = originalPoint;
		emitTriangle(v1, v2, v3);
		if (!join) { //create a face mimicing the base and the quad to close the hole in between
			vec4 v4 = v1 + extrudeVector;
			vec4 v5 = v2 + extrudeVector;
			vec4 v6 = v3 + extrudeVector;
			emitTriangle(v4, v5, v6);
			emitQuad(v1, v2, v4, v5);
		} else { //create a triangle to close the hole between the base and the summit
			vec4 v4 = v3 + extrudeVector;
			emitTriangle(v1, v2, v4);
		}
	}
}

void main() {
    //emitShapeExtrude(int(u_time * 0.25) % 4 + 3, vec4(0.0, 0.125, 0.25, .0), gl_in[0].gl_Position, vec2(0.4), false);
    //emitShapeExtrude(3, vec4(0.0, 0.125, 0.25, .0), gl_in[0].gl_Position  - gl_in[0].gl_Position + ampersandCurve(gl_in[0].gl_Position.xy), vec2(0.4), true); //alongCurve
    if (geometryGenerateSomething[0] == 1) {
	emitShapeExtrude(int(u_time * 0.25) % 4 + 3, vec4(0.0, 0.125, 0.25, .0), gl_in[0].gl_Position, vec2(0.4), true);
    }
    //emitTriangle(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
}
