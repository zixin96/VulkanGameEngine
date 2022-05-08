#version 450

// output color to the first (and only) framebuffer at index 0
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat2 transform;
    vec2 offset;
    vec3 color;
} push;

void main() {
    outColor = vec4(push.color, 1.0);
}