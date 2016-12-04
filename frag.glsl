#version 400 core

in vec3 position;
out vec4 color;

vec4 tridentCurve(vec2 p) {
    float a = -2;
    float b = -2;
    float c = -2;
    float d = -2;

    float color = clamp(p.x * p.y + a * pow(p.x, 3.0) + b * pow(p.x, 2.0 + c * p.x) - d, 0.0, 1.0);
    return vec4(color, color, color, 1.0);
    //xy+ax3+bx2+cx=d
}

vec4 ampersandCurve(vec2 p) {
    float color = clamp((pow(p.y, 2.0) - pow(p.x, 2.0)) * (p.x - 1.0) * (2.0 * p.x - 3.0) - 4.0 * pow(pow(p.x, 2.0) + pow(p.y, 2.0) - 2.0 * p.x, 2.0), 0.0, 1.0);
    return vec4(color, color, color, 1.0);
    //(y2 - x2)(x-1)(2x-3)=4(x2+y2-2x)2
}

vec4 bicuspidCurve(vec2 p) {
    float a = 2.5;
    float color = clamp((pow(p.x, 2.0) - pow(a, 2.0)) * pow(p.x - a, 2.0) + pow(pow(p.y, 2.0) - pow(a, 2.0), 2.0), 0.0, 1.0);
    return vec4(color, color, color, 1.0);
    //(x2-a2)(x-a)2+(y2-a2)2=0
}

void main() {
    //color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    //color = tridentCurve(position.xy);
    //color = bicuspidCurve(position.xy);
    color = ampersandCurve(position.xy);
}
