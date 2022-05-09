#version 450

layout(location = 0) in vec2 fragOffset;

// output color to the first (and only) framebuffer at index 0
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    vec4 ambientLightColor; // w is intensity
    vec3 lightPosition;
    vec4 lightColor; // w is light intensity
} ubo;


void main() {
    // make the quad circle-like
    float dis = sqrt(dot(fragOffset, fragOffset));
    if(dis >= 1.0) { discard;}

    outColor = vec4(ubo.lightColor.xyz, 1.0);
}