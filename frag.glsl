#version 400 core

in vec3 position;
out vec3 fragColor;
varying vec2 v_texcoord;
uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}
vec2 add(vec2 d1, vec2 d2) {
    return (d1.x<d2.x) ? d1 : d2;
}

float sdSphere( vec3 p, float s ) {
    return length(p)-s;
}

float udRoundBox(vec3 p, vec3 b, float r) {
    return length(max(abs(p)-b,0.0))-r;
}

vec2 scene(in vec3 pos) {
    vec2 r = vec2(sdSphere(pos - vec3(0.0, cos(u_time), 0.0), 0.25), 46.9);
    r = add(r, vec2(sdSphere(pos - vec3(sin(u_time), 0.25, 0.0), 0.25), 26.9));
    r = add(r, vec2(udRoundBox(pos - vec3(0.0, -1.5, 0.0), vec3(2., 0.2, 2.2), 0.1), 10.2));
    return r;
}

#define MAX_RAYMARCH_ITERATIONS 48
// IQ Raymarching www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
vec2 castRay(in vec3 rayOrigin, in vec3 rayDirection, out float complexity) {
    const float depthMin = 1.0;
    const float depthMax= 20.0;

    float depth = depthMin;
    float m = -1.0; // what is that ?
    complexity = 0;
    for(int i=0; i < MAX_RAYMARCH_ITERATIONS; i++) {
	float precis = 0.00005 * depth;
	vec2 res = scene(rayOrigin + rayDirection * depth);
	if(res.x < precis || depth > depthMax) {
	    break;
	}
	depth += res.x;
	m = res.y;
	++complexity;
    }
    complexity /= MAX_RAYMARCH_ITERATIONS;

    if(depth > depthMax) {
	m =-1.0;
    }
    return vec2(depth, m);
}

void main() {
    //float d = sdSphere(vec3(10, 0.0, 0.0), 5);
    vec3 camera = vec3(1.0, 2.0, -5);
    vec2 coord = -1.0 + 2.0 * gl_FragCoord.xy / u_resolution;
    coord.y /= u_resolution.x / u_resolution.y;
    vec3 dir = setCamera(camera, vec3( -0.5, -0.4, 0.5 ), 0.0) * normalize(vec3(coord, 2.));

    float complexity;
    vec2 data = castRay(camera, dir, complexity);
    vec3 pos = camera + data.x * dir;
#if 0
    fragColor = vec3(complexity); return;
#endif
    if(data.y < 1.5) {
	fragColor = vec3(0.01, 0.01, 0.02);
    } else {
	fragColor = 0.45 + 0.35 * sin( vec3(0.05,0.08,0.10) * (data.y - 1.0) );
    }
}
