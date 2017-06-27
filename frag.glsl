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

struct Light {
    vec3 pos;
    vec3 color;
};
#define lightSizeArray 1
Light lights[lightSizeArray];

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 cookTorrance(vec3 N, vec3 H, vec3 V, vec3 L, float roughness, vec3 F) {

    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      

    vec3 DFG = NDF * G * F;
    return DFG / (4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001);
}

vec3 HeTorranceSillionGreenberg(vec3 N, vec3 H, vec3 V, vec3 L, vec3 albedo, float roughness, float metallic, float IOR) {
    //vec3 F0 = mix(vec3(0.04), albedo, metallic);
    // IOR examples
    //water 1.33 
    //plastic 1.45 
    //glass 1.5-1.8 
    //diamond 2.4 
    //compound materials like wood, stone, concrete etc 3-6 
    //metals 20-100 
    float F = fresnel(N, V, IOR);
    float patchDiameter = 0.5f; // ??
    float tetha = atan(patchDiameter) * 2;
    float dw = sin(tetha) * patchDiameter * patchDiameter;
    float S = GeometrySmith(N, V, L, roughness); // micro shadowing function
    float G = 1.0f; // ??
    vec3 delta = vec3(1.0f);
    return (F * F * exp(-G) * S) / (cos(dot(N, H)) * dw) * delta;
}

vec3 pbr(vec3 viewDir, vec3 hitPosition, vec3 surfaceNormal, vec3 albedo, vec3 irradiance, float ao, float roughness, float metallic) {
    vec3 N = surfaceNormal;
    vec3 V = viewDir;


    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    // reflectance equation
    vec3 Lo = vec3(0.0);

    vec3 kD;
    for(int i = 0; i < lightSizeArray; ++i) {
	// calculate per-light radiance
	vec3 L = normalize(lights[i].pos - hitPosition);
	vec3 H = normalize(V + L);
	float distance    = length(lights[i].pos - hitPosition);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance     = lights[i].color * attenuation;        

	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
	vec3 brdf = cookTorrance(N, H, V, L, roughness, F);
	//vec3 brdf = HeTorranceSillionGreenberg(N, H, V, L, albedo, roughness, metallic, 1.45);

	// add to outgoing radiance Lo
	float NdotL = max(dot(N, L), 0.0);                
	vec3 kS = F;
	kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	Lo += (kD * albedo / PI + brdf) * radiance * NdotL; 
    }   

    vec3 ambient = vec3(0.03) * albedo * ao;
    //vec3 ambient = (kD * diffuse + specular) * ao; 
    //vec3 ambient = (irradiance * kD * albedo) * ao; 
    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    return color;
}

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
    return (d1.x < d2.x) ? d1 : d2;
}
distColour sub(distColour d1, distColour d2) {
    return (-d1.x > d2.x) ? d1 : d2;
}
distColour inter(distColour d1, distColour d2) {
    return (d1.x > d2.x) ? d1 : d2;
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
    //distColour r = distColour(sdSphere(pos - vec3(0.0, cos(u_time), 0.0), sin(u_time)), colour(0.9, 0.2, 0.2));
    //r = add(r, distColour(sdSphere(pos - vec3(sin(u_time), 0.25, 0.0), 0.25), colour(0.2, 1.0, 0.4)));
    //r = add(r, distColour(udRoundBox(pos - vec3(0.0, -1.0, 0.0), vec3(2., 0.2, 2.2), 0.1), colour(0.3, 0.7, 0.8)));
    //r = add(r, distColour(udBox(pos - vec3 (-2.0, 0.0, 0.0), vec3(0.1, 5.0, 4.0)), colour(0.1,0.8, 0.3)));
    //r = add(r, distColour(udBox(pos - vec3 (2.0, 0.0, 0.0), vec3(0.1, 5.0, 4.0)), colour(0.4, 0.1, 0.1)));
    //r = add(r, distColour(udBox(pos - vec3 (0.0, 0.0, 2.0), vec3(5.0, 5.0, 0.1)), colour(0.1, 0.0, 0.7)));

    distColour r = distColour(udBox(pos - vec3(0.0, -0.9, 0.0), vec3(0.9, 0.1, 9.1)), colour(0.9, 0.9, 0.2));
    distColour s = distColour(udBox(pos - vec3 (-0.3, sin(u_time), 0.1), vec3(0.2, 0.8, 0.3)), colour(0.1, 0.0, 0.7));

    return sub(r, s);
    //return add(s,r);
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
    vec3 viewDir = setCamera(camera, vec3(-0.5, -0.4, 0.5 ), 0.0) * normalize(vec3(coord, 2.));

    float complexity;
    distColour data = castRay(camera, viewDir, complexity);

    vec3 hitPosition = camera + data.x * viewDir;
    vec3 normal = Normal(hitPosition);
    float AO = AO(hitPosition, normal);
    distColour reflection;
    float refComplexity = 1.0f;
    vec3 refDir = reflect(viewDir, normal);
    reflection = castRay(hitPosition, refDir, refComplexity);
    vec3 refHitPos = hitPosition + reflection.x * refDir;
#if 0
    fragColor = vec3(complexity); return;
#endif
    //data.y = mix(reflection.y, data.y, (sin(u_time) / 2.0) + 0.5); // testing reflections
    //data.yzw = mix(reflection.yzw, data.yzw, (sin(u_time) / 2.0) + 0.5); // testing reflections



    //fragColor = data.yzw;
    //fragColor = mix(fragColor * 0.4, fragColor, AO);

    float roughness = 0.1;
    float metallic = 0.1;
    lights[0].pos = vec3(0.9, 0.1, -0.9);
    lights[0].color = vec3(3.0, 3.0, 10.0);
    fragColor = pbr(viewDir, hitPosition, normal, data.yzw, reflection.yzw, AO, roughness, metallic);
}
