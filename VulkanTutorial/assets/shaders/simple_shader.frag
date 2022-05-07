#version 450

// output color to the first (and only) framebuffer at index 0
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 1.0, 1.0);
}