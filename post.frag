#version 450
#define PI 3.14159

uniform sampler2D gPositionDepth;
uniform sampler2D gNormal;
uniform sampler2D gColour;

in vec3 position;
out vec3 fragColor;
layout (location = 0) uniform float u_time;
layout (location = 1) uniform vec2 u_resolution;
layout (location = 2) uniform vec2 u_mouse;

vec2 lens_distortion(vec2 r, float alpha) {
	return r * (1.0 - alpha * dot(r, r));
}

void main() {
    vec2 coord = -1.0 + gl_FragCoord.xy / u_resolution;
    vec2 remapped = coord * 2.0 + 1.0;

    // lens distortion
    fragColor = texture(gColour, (lens_distortion(remapped, 0.2) / 2.0) + 0.5).xyz;

    // chromatic aberation
    float chromaticAmount = clamp(-0.005, 0.005, smoothstep(0.35, 1.8, length(remapped)));
    vec2 uvRed = coord;
    vec2 uvBlue = coord;
    float s = abs(sin(u_time * 1.0)) * chromaticAmount;
    uvRed.x += s;
    uvBlue.x -= s;

    fragColor =  texture(gColour, coord).xyz;

    fragColor.r = texture(gColour, uvRed).r;
    fragColor.b = texture(gColour, uvBlue).b;

    // vigneting
    fragColor *= 1.0 - smoothstep(0.15, 1.3, length(remapped));
}
