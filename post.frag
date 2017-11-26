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
float speed;

void main() {
    vec2 coord = -1.0 + gl_FragCoord.xy / u_resolution;
    fragColor = texture(gColour, coord).xyz;
}
