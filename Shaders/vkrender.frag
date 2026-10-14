#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 tex = texture(texSampler, inUV);

    // Cheap directional light (no normalize on constant)
    vec3 N = normalize(inNormal);
    const vec3 L = vec3(0.4472, 0.8944, 0.0);  // pre-normalized (1,2,0)
    float ndotl = max(dot(N, L), 0.0);
    float light = 0.35 + 0.65 * ndotl;

    outColor = vec4(tex.rgb * light, 1.0);
}