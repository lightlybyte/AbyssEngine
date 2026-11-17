#version 450

// Minimal terrain fragment shader
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple diffuse lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    
    // Gradient based on UV for visual interest
    vec3 baseColor = mix(vec3(0.2, 0.4, 0.2), vec3(0.6, 0.5, 0.4), vUV.x);
    vec3 color = baseColor * (0.3 + 0.7 * diff);
    
    outColor = vec4(color, 1.0);
}

