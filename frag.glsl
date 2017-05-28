#version 400 core

#define colour vec3
#define distColour vec4

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
distColour add(distColour d1, distColour d2) {
    return (d1.x<d2.x) ? d1 : d2;
}

float sdSphere(vec3 p, float s) {
    return length(p)-s;
}

float udRoundBox(vec3 p, vec3 b, float r) {
    return length(max(abs(p)-b,0.0))-r;
}
float udBox( vec3 p, vec3 b )
{
    return length(max(abs(p)-b,0.0));
}

distColour scene(in vec3 pos) {
    distColour r = distColour(sdSphere(pos - vec3(0.0, cos(u_time), 0.0), 0.25), colour(0.9, 0.2, 0.2));
    r = add(r, distColour(sdSphere(pos - vec3(sin(u_time), 0.25, 0.0), 0.25), colour(0.2, 1.0, 0.4)));
    r = add(r, distColour(udRoundBox(pos - vec3(0.0, -1.0, 0.0), vec3(2., 0.2, 2.2), 0.1), colour(0.3, 0.7, 0.8)));
    r = add(r, distColour(udBox(pos - vec3 (-2.0, 0.0, 0.0), vec3(0.1, 5.0, 4.0)), colour(0.1,0.8, 0.3)));
    r = add(r, distColour(udBox(pos - vec3 (2.0, 0.0, 0.0), vec3(0.1, 5.0, 4.0)), colour(0.4, 0.1, 0.1)));
    r = add(r, distColour(udBox(pos - vec3 (0.0, 0.0, 2.0), vec3(5.0, 5.0, 0.1)), colour(0.1, 0.0, 0.7)));
    return r;
}

vec3 Normal( in vec3 pos )
{
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize(e.xyy*scene(pos + e.xyy).x + 
	    e.yyx*scene(pos + e.yyx).x + 
	    e.yxy*scene(pos + e.yxy).x + 
	    e.xxx*scene(pos + e.xxx).x );
    /*
       vec3 eps = vec3( 0.0005, 0.0, 0.0 );
       vec3 nor = vec3(
       map(pos+eps.xyy).x - map(pos-eps.xyy).x,
       map(pos+eps.yxy).x - map(pos-eps.yxy).x,
       map(pos+eps.yyx).x - map(pos-eps.yyx).x );
       return normalize(nor);
     */
}

float AO(in vec3 pos, in vec3 nor) {
    float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
	float hr = 0.01 + 0.12*float(i)/4.0;
	vec3 aopos =  nor * hr + pos;
	float dd = scene(aopos).x;
	occ += -(dd-hr)*sca;
	sca *= 0.95;
    }
    return clamp(1.0 - 3.0 * occ, 0.0, 1.0);
}

#define MAX_RAYMARCH_ITERATIONS 48
// IQ Raymarching www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
distColour castRay(in vec3 rayOrigin, in vec3 rayDirection, out float complexity) {
    const float depthMin = 1.0;
    const float depthMax= 20.0;

    float depth = depthMin;
    colour m = colour(0.0, 0.0, 0.0);
    complexity = 0;
    for(int i=0; i < MAX_RAYMARCH_ITERATIONS; i++) {
	float precis = 0.00005 * depth;
	distColour res = scene(rayOrigin + rayDirection * depth);
	if(res.x < precis || depth > depthMax) {
	    break;
	}
	depth += res.x;
	m = res.yzw;
	++complexity;
    }

    complexity /= MAX_RAYMARCH_ITERATIONS;

    if(depth > depthMax) {
	m = colour(0.0, 0.0, 0.0);
    }
    return distColour(depth, m);
}

void main() {
    //float d = sdSphere(vec3(10, 0.0, 0.0), 5);
    vec3 camera = vec3(1.0, 2.0, -5);
    vec2 coord = -1.0 + 2.0 * gl_FragCoord.xy / u_resolution;
    coord.y /= u_resolution.x / u_resolution.y;
    vec3 dir = setCamera(camera, vec3( -0.5, -0.4, 0.5 ), 0.0) * normalize(vec3(coord, 2.));

    float complexity;
    distColour data = castRay(camera, dir, complexity);

    vec3 hitPosition = camera + data.x * dir;
    vec3 normal = Normal(hitPosition);
    float AO = AO(hitPosition, normal);
    distColour reflection;
    float refComplexity = 1.0f;
    reflection = castRay(hitPosition, reflect(dir, normal), refComplexity);
#if 0
    fragColor = vec3(complexity); return;
#endif
    //data.y = mix(reflection.y, data.y, (sin(u_time) / 2.0) + 0.5); // testing reflections
    data.yzw = mix(reflection.yzw, data.yzw, 0.6); // testing reflections
    fragColor = data.yzw;
    fragColor = mix(fragColor * 0.4, fragColor, AO);
}
