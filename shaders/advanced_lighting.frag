#version 450

// Minimal advanced lighting fragment shader
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple diffuse lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 color = vColor.rgb * (0.3 + 0.7 * diff);
    outColor = vec4(color, 1.0);
}

