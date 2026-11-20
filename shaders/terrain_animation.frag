// Realistic terrain animation shader with earth-like lighting
#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

// Uniform block for time (Vulkan requires all uniforms in blocks)
layout(set = 0, binding = 1) uniform TimeUBO {
    float time;
} ubo;

const vec3 sunDir = normalize(vec3(0.4, 0.9, 0.3));
const vec3 ambient = vec3(0.12, 0.15, 0.22);
const vec3 sunCol = vec3(1.0, 0.92, 0.78);
const vec3 fogCol = vec3(0.7, 0.78, 0.88);

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = sunDir;
    
    float diff = max(dot(N, L), 0.0);
    
    // Simple animated water effect for low areas
    vec3 V = normalize(-vWorldPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.4;
    
    // Height-based coloring with animation
    float height = fract(vWorldPos.y * 0.05 + ubo.time * 0.01);
    vec3 sand = vec3(0.76, 0.68, 0.52);
    vec3 grass = vec3(0.18, 0.38, 0.15);
    vec3 water = vec3(0.1, 0.3, 0.5);
    
    vec3 color = mix(water, sand, smoothstep(0.0, 0.2, height));
    color = mix(color, grass, smoothstep(0.2, 0.5, height));
    
    // Distance fog
    float fogF = smoothstep(30.0, 200.0, length(vWorldPos));
    
    vec3 lit = color * (ambient + sunCol * diff) + sunCol * spec;
    lit = mix(lit, fogCol, fogF);
    
    outColor = vec4(lit, 1.0);
}
