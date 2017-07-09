#version 430
#define colour vec3
#define distColour vec4
#define PI 3.14159

in vec3 position;
in vec2 v_texcoord;
out vec3 fragColor;
uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
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

mat3 setCamera(in lowp vec3 ro, in lowp vec3 ta, lowp float cr) {
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
lowp float add(lowp float d1, lowp float d2) {
    return min(d1, d2);
}
lowp float sub(lowp float d1, lowp float d2) {
    return max(-d1, d2);
}
lowp float inter(lowp float d1, lowp float d2) {
    return max(d1, d2);
}


lowp float sdSphere(lowp vec3 p, lowp float s) {
    return length(p)-s;
}

lowp float udRoundBox(lowp vec3 p, lowp vec3 b, lowp float r) {
    return length(max(abs(p)-b,0.0))-r;
}
lowp float Box(lowp vec3 p, lowp vec3 b) {
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
    //return length(max(abs(p)-b,0.0)); // unsigned version
}

distColour scene(in vec3 pos) {
    // road
    roadPos = pos - vec3(sin(pos.z) / 2.0f, -1.6, 0.0);
    roadPos.z = mod(roadPos.z, 2.0f);
    float box =  Box(roadPos, vec3(0.9, 0.3, 9.0));
    float obox = Box(roadPos + vec3(0.0, -0.5, 0.0), vec3(0.8, 0.4, 9.1));


    colour roadColour = vec3(0.0, 0.0, 0.0);
    if (roadPos.x < 0.77f && roadPos.x > -0.77f) {
	lowp float time = u_time * ((sin(u_time) + (sin(2.0f * u_time) / 2.0f)) / 2.0f) + 0.2;
	roadColour.b = max(sin((abs(roadPos.x * 5.0f) - time)), 0.0f);
	roadColour.b = pow((roadColour.b), 5);
	roadColour.rg = vec2(mix(0.0, 1.0, smoothstep(0.5, 1.0, roadColour.b)));
        roadColour *= 2.0f;
    }
    distColour road = distColour(sub(obox.x, box.x), roadColour + colour(0.1, 0.1, 0.4));
    distColour character = distColour(sdSphere(pos - vec3(sin(pos.z) / 2.0f, -1., camera.z + 10.0), 0.4), colour(0.3));
    return add(road, character);
}

vec3 Normal(in vec3 pos) {
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize(e.xyy*scene(pos + e.xyy).x + 
	    e.yyx*scene(pos + e.yyx).x + 
	    e.yxy*scene(pos + e.yxy).x + 
	    e.xxx*scene(pos + e.xxx).x );
}

colour AO(in vec3 pos, in vec3 nor) {
    lowp float occ = 0.0;
    lowp float sca = 1.0;
    distColour dd;
    for( int i=0; i<5; i++ )
    {
	lowp float hr = 0.01 + 0.12*float(i)/4.0;
	lowp vec3 aopos =  nor * hr + pos;
	dd = scene(aopos);
	occ += -(dd.x-hr)*sca;
	sca *= 0.95;
    }
    float ao = 1.0 - 3.0 * occ;
    if (length(dd.yzw) > 1.0f && ao < 1.) {
        return dd.yzw * (1 - ao); 
    } else {
        return clamp(ao, 0.0, 1.0).xxx;
    }
}

//#define MAX_RAYMARCH_ITERATIONS 48 // enough for tests
#define MAX_RAYMARCH_ITERATIONS 300 // production level
// IQ Raymarching www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
distColour castRay(in vec3 rayOrigin, in vec3 rayDirection, out lowp float complexity) {
    const lowp float depthMin = 1.0;
    const lowp float depthMax= 60.0;

    lowp float depth = depthMin;
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

    const lowp float depthFadeSize = 20.0f;
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
    vec3 viewDir = setCamera(camera, camera + vec3(0.0, -2.5, 20.0), 0.0) * normalize(vec3(coord, 2.));

    lowp float complexity;
    distColour data = castRay(camera, viewDir, complexity);

    vec3 hitPosition = camera + data.x * viewDir;
    vec3 normal = vec3(0.0f);
    colour ao = vec3(1.0);
    distColour reflection;
    lowp float refComplexity = 1.0f;
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
    fragColor = vec3(complexity); return;
#endif
    //data.y = mix(reflection.y, data.y, (sin(u_time) / 2.0) + 0.5); // testing reflections
    //data.yzw = mix(reflection.yzw, data.yzw, (sin(u_time) / 2.0) + 0.5); // testing reflections

    //fragColor = data.yzw;
    //fragColor = mix(fragColor * 0.4, fragColor, AO);

    lowp float fresnel = fresnel(normal, viewDir, 1.4) + 0.1;
    fragColor = fresnel.xxx;
    fragColor = mix(data.yzw, reflection.yzw, fresnel) * ao;
}
