// Realistic earth-like diffuse lighting fragment shader with atmosphere
#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

// Earth-like sun position (high angle for midday)
const vec3 sunDirection = normalize(vec3(0.3, 0.8, 0.5));
const vec3 ambientLight = vec3(0.15, 0.18, 0.25); // Slight blue sky ambient
const vec3 sunColor = vec3(1.0, 0.95, 0.85);      // Warm sunlight
const vec3 fogColor = vec3(0.65, 0.75, 0.85);     // Atmospheric haze color

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(sunDirection);
    
    // Diffuse lambert shading
    float diff = max(dot(N, L), 0.0);
    
    // Specular (Blinn-Phong) for wet/earth surfaces
    vec3 viewDir = normalize(-vWorldPos);
    vec3 halfDir = normalize(L + viewDir);
    float spec = pow(max(dot(N, halfDir), 0.0), 32.0) * 0.3;
    
    // Ground fog / distance fade
    float dist = length(vWorldPos);
    float fogFactor = smoothstep(50.0, 150.0, dist);
    
    // Mix terrain colors based on UV (latitude approximation)
    float latitude = abs(vUV.y - 0.5) * 2.0;
    vec3 soilColor = vec3(0.35, 0.28, 0.20);     // Brown dirt
    vec3 grassColor = mix(vec3(0.15, 0.35, 0.12), vec3(0.25, 0.45, 0.18), latitude);
    vec3 rockColor = vec3(0.45, 0.42, 0.38);
    vec3 baseColor = mix(grassColor, soilColor, step(0.6, latitude));
    baseColor = mix(baseColor, rockColor, step(0.85, latitude));
    
    // Apply lighting
    vec3 litColor = baseColor * (ambientLight + sunColor * diff) + sunColor * spec;
    
    // Fog integration
    vec3 finalColor = mix(litColor, fogColor, fogFactor);
    
    outColor = vec4(finalColor, 1.0);
}
