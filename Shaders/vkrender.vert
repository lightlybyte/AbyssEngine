#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outWorldPos;

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 world = vec4(inPos, 1.0);
    gl_Position = ubo.proj * ubo.view * world;

    outNormal = inNormal;
    outUV = inUV;
    outWorldPos = inPos;
}