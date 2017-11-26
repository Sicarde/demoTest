#version 450
#define distColour vec4
#define PI 3.14159

layout (location = 0) out vec4 gPositionDepth;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gColour;

in vec3 position;
//out vec3 gColour;
layout (location = 0) uniform float u_time;
layout (location = 1) uniform vec2 u_resolution;
layout (location = 2) uniform vec2 u_mouse;
float speed;
vec3 camera;
vec3 roadPos;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlick(float cosTheta) {
    return fresnelSchlick(cosTheta, vec3(0.04));
}

float fresnel(vec3 normal, vec3 viewDir, float IOR) {
    float R0 = ((-1/(IOR+2.6)) * 2.4) + 0.75;
    return  R0 + (1.0f - R0) * exp(((1.0f - dot(-viewDir, normal)) - 1) * 4.6);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

mat3 setCamera(in  vec3 ro, in  vec3 ta,  float cr) {
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

distColour add(distColour d1, distColour d2) {
    // return mix(a, b, step(b.x, a.x));
    return (d1.x < d2.x) ? d1 : d2;
}
distColour sub(distColour d1, distColour d2) {
    return (-d1.x > d2.x) ? distColour(-d1.x, d1.yzw) : distColour(d2.x, d2.yzw);
}
distColour inter(distColour d1, distColour d2) {
    return (d1.x > d2.x) ? d1 : d2;
}
float add( float d1,  float d2) {
    return min(d1, d2);
}
float sub( float d1,  float d2) {
    return max(-d1, d2);
}
float inter( float d1,  float d2) {
    return max(d1, d2);
}


float sdSphere( vec3 p,  float s) {
    return length(p)-s;
}

float udRoundBox( vec3 p,  vec3 b,  float r) {
    return length(max(abs(p)-b,0.0))-r;
}
float Box( vec3 p,  vec3 b) {
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
    //return length(max(abs(p)-b,0.0)); // unsigned version
}

distColour tentacle(vec3 pos,  float height) {
    vec3 c = vec3(1.0, 0.0, 0.0);
    float radius = mix(0.2, 0.0, max((pos.y - 0.0) / height, 0.0));
    radius += cos(pos.y * 4.0) / 50.0f;
    float dist = length(pos.xz) - radius;

    return distColour(dist, c);
}

distColour character(vec3 pos) {
    return distColour(sdSphere(pos - vec3(sin(pos.z) / 2.0f, -1., camera.z + 10.0), 0.4), vec3(0.3));
}

distColour scene(in vec3 pos) {
    float distDebugPosCubes = Box(vec3(vec3(2.0) - mod(pos, 4.0)), vec3(abs(sin(u_time*4.0)) * 0.3));
    distColour debugPosCubes = distColour(distDebugPosCubes, pos);
    // road
    roadPos = pos - vec3(sin(pos.z) / 2.0f, -1.6, 0.0);
    roadPos.z = mod(roadPos.z, 2.0f);
    float box =  Box(roadPos, vec3(0.9, 0.3, 9.0));
    float obox = Box(roadPos + vec3(0.0, -0.5, 0.0), vec3(0.8, 0.4, 9.1));

    distColour t = tentacle(vec3(pos.x, roadPos.y, mod(pos.z, 6.0f)) - vec3(1.5, 0.0, 3.0), 4.0);
    //return add(t, debugPosCubes);

    vec3 roadColour = vec3(0.0, 0.0, 0.0);
    if (roadPos.x < 0.77f && roadPos.x > -0.77f) {
        float time = u_time * ((sin(u_time) + (sin(2.0f * u_time) / 2.0f)) / 2.0f) + 0.2;
        roadColour.b = max(sin((abs(roadPos.x * 5.0f) - time)), 0.0f);
        roadColour.b = pow((roadColour.b), 5);
        roadColour.rg = vec2(mix(0.0, 1.0, smoothstep(0.5, 1.0, roadColour.b)));
        roadColour *= 2.0f;
    }
    distColour road = distColour(sub(obox, box), roadColour + vec3(0.1, 0.1, 0.4));
    distColour character = character(pos);

    distColour rc = add(road, character);
    //return add(rc, t);
    return add(add(rc, t), debugPosCubes);
    //return add(road, character);
}

vec3 Normal(in vec3 pos) {
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize(e.xyy*scene(pos + e.xyy).x + 
            e.yyx*scene(pos + e.yyx).x + 
            e.yxy*scene(pos + e.yxy).x + 
            e.xxx*scene(pos + e.xxx).x );
}

vec3 AO(in vec3 pos, in vec3 nor) {
    float occ = 0.0;
    float sca = 1.0;
    distColour dd;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01 + 0.12*float(i)/4.0;
        vec3 aopos =  nor * hr + pos;
        dd = scene(aopos);
        occ += -(dd.x-hr)*sca;
        sca *= 0.95;
    }
    float ao = 1.0 - 3.0 * occ;
    if (length(dd.yzw) > 1.0f && ao < 1.) {
        return dd.yzw * (1 - ao); 
    } else {
        return vec3(clamp(ao, 0.0, 1.0));
    }
}

//#define MAX_RAYMARCH_ITERATIONS 48 // enough for tests
#define MAX_RAYMARCH_ITERATIONS 2048 // production level
// IQ Raymarching www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
distColour castRay(in vec3 rayOrigin, in vec3 rayDirection, out  float complexity) {
    const  float depthMin = 1.0;
    const  float depthMax= 60.0;

    float depth = depthMin;
    vec3 m = vec3(0.0, 0.0, 0.0);
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

    const  float depthFadeSize = 20.0f;
    if (depth >= depthMax) {
        m *= 0.0f;
    } else if (depth > (depthMax - 20)) {
        m *= smoothstep(depthMax, depthMax - 20, depth);
    }
    return distColour(depth, m);
}

void main() {
    speed = 1; // default
    highp float baseTime = u_time * speed;

    camera = vec3(0.0, 0.3, baseTime);
    vec2 coord = -1.0 + 2.0 * gl_FragCoord.xy / u_resolution;
    coord.y /= u_resolution.x / u_resolution.y;
    vec3 viewDir = setCamera(camera, camera + vec3(0.0, 0.5, 20.0), 0.0) * normalize(vec3(coord, 2.));

    float complexity;
    distColour data = castRay(camera, viewDir, complexity);

    vec3 hitPosition = camera + data.x * viewDir;
    vec3 normal = vec3(0.0f);
    vec3 ao = vec3(1.0);
    distColour reflection;
    float refComplexity = 1.0f;
    if (data.x < 59.0) {
        normal = Normal(hitPosition);
        ao = AO(hitPosition, normal);
        vec3 refDir = reflect(viewDir, normal);
        reflection = castRay(hitPosition, refDir, refComplexity);
        //vec3 refHitPos = hitPosition + reflection.x * refDir;
    } else {
        reflection = distColour(60.0f, 0.0, 0.0, .0);
    }
#if 0
    gColour = vec3(complexity); return;
#endif
    //data.y = mix(reflection.y, data.y, (sin(u_time) / 2.0) + 0.5); // testing reflections
    //data.yzw = mix(reflection.yzw, data.yzw, (sin(u_time) / 2.0) + 0.5); // testing reflections

    //gColour = data.yzw;
    //gColour = mix(gColour * 0.4, gColour, AO);

    float fresnel = fresnel(normal, viewDir, 1.4) + 0.1;
    gColour = vec3(fresnel);
    gColour = mix(data.yzw, reflection.yzw, fresnel) * ao;
    gPositionDepth.xyz = camera * (viewDir * data.x);
    gPositionDepth.w = data.x;
    gNormal = normal;
}

