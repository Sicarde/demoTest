#version 400 core

#define colour vec3
#define distColour vec4
#define PI 3.14159

in vec3 position;
out vec3 fragColor;
varying vec2 v_texcoord;
uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;

//-------------------------- PBR functions -----------------------

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlick(float cosTheta) {
    return fresnelSchlick(cosTheta, vec3(0.04));
}

float fresnel(vec3 normal, vec3 dir, float IOR) {
    float R0 = ((-1/(IOR+2.6)) * 2.4) + 0.75;
    return  R0 + (1.0f - R0) * exp(((1.0f - dot(-dir, normal)) - 1) * 4.6);
}

vec3 orenNayar(vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, vec3 albedo) {
    float NdotL = dot(lightDirection, surfaceNormal);
    float NdotV = dot(surfaceNormal, viewDirection);

    float s = dot(lightDirection, viewDirection) - NdotL * NdotV;
    float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

    float sigma2 = roughness * roughness;
    vec3 A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

float ggx(vec3 surfaceNormal, vec3 viewDir, vec3 lightDir, float roughness, float F0) { // Trowbridge-Reitz
    float alpha = roughness * roughness;
    vec3 H = normalize(lightDir - viewDir);
    float dotLH = max(0.0, dot(lightDir, H));
    float dotNH = max(0.0, dot(surfaceNormal, H));
    float dotNL = max(0.0, dot(surfaceNormal, lightDir));
    float alphaSqr = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (3.141592653589793 * denom * denom);
    float F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 5.0);
    //float F = fresnelSchlick(dotLH, vec3(F0, F0, F0));
    float k = 0.5 * alpha;
    float k2 = k * k;
    return dotNL * D * F / (dotLH*dotLH*(1.0-k2)+k2);
}

struct Light {
    vec3 pos;
    vec3 color;
};

//-------------------------- PBR functions END -------------------


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
    distColour r = distColour(sdSphere(pos - vec3(0.0, cos(u_time), 0.0), sin(u_time)), colour(0.9, 0.2, 0.2));
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
    //data.yzw = mix(reflection.yzw, data.yzw, (sin(u_time) / 2.0) + 0.5); // testing reflections


    //orenNayar(); // diffuse only
    //ggx(); // diffuse only

    //fragColor = data.yzw;
    //fragColor = mix(fragColor * 0.4, fragColor, AO);

    Light l1;
    l1.pos = vec3(0.75, 0.6, 0.2);
    l1.color = vec3(0.8, 0.8, 0.8);
    float roughness = 0.1;
    float ggx = ggx(normal, dir, normalize(l1.pos - hitPosition ), 1.0, 0.44);
    vec3 nayar = orenNayar(normalize(l1.pos - hitPosition ), dir, normal, roughness, data.yzw);
    fragColor = mix(nayar, reflection.yzw, ggx);
}
