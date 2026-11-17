#version 450

// Minimal animation fragment shader
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
    // Simple diffuse lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    
    // Checkered pattern for visibility
    vec2 check = step(0.5, fract(vUV * 4.0));
    float checker = mix(check.x, check.y, check.y);
    vec3 color = mix(vec3(0.8, 0.6, 0.4), vec3(0.3, 0.3, 0.3), checker);
    
    color *= (0.3 + 0.7 * diff);
    outColor = vec4(color, 1.0);
}

