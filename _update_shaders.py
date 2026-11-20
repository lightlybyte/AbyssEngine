import subprocess
import os

os.chdir(r'c:/AbyssEngine')

# 1. Update shaders to realistic earth-like lighting
shaders = {
    'Shaders/advanced_lighting.frag': '''// Realistic earth-like diffuse lighting fragment shader with atmosphere
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
''',
    'Shaders/terrain.frag': '''// Realistic terrain shader with earth-like coloring and atmospheric scattering
#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 outColor;

const vec3 sunDir = normalize(vec3(0.4, 0.9, 0.3));
const vec3 ambient = vec3(0.12, 0.15, 0.22);
const vec3 sunCol = vec3(1.0, 0.92, 0.78);
const vec3 fogCol = vec3(0.7, 0.78, 0.88);

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = sunDir;
    
    float diff = max(dot(N, L), 0.0);
    
    // View direction for specular
    vec3 V = normalize(-vWorldPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.25;
    
    // Height-based terrain coloring (simple fake altitude)
    float height = fract(vWorldPos.y * 0.1);
    vec3 sand = vec3(0.76, 0.68, 0.52);
    vec3 grass = vec3(0.18, 0.38, 0.15);
    vec3 forest = vec3(0.08, 0.22, 0.06);
    vec3 stone = vec3(0.48, 0.45, 0.42);
    vec3 snow = vec3(0.95, 0.95, 0.97);
    
    vec3 color;
    if (height < 0.25) color = sand;
    else if (height < 0.45) color = grass;
    else if (height < 0.65) color = forest;
    else if (height < 0.82) color = stone;
    else color = snow;
    
    // Distance fog
    float fogF = smoothstep(30.0, 200.0, length(vWorldPos));
    
    vec3 lit = color * (ambient + sunCol * diff) + sunCol * spec;
    lit = mix(lit, fogCol, fogF);
    
    outColor = vec4(lit, 1.0);
}
''',
    'Shaders/terrain_animation.frag': '''// Realistic terrain animation shader with earth-like lighting
#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

uniform float time;

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
    
    // Height-based coloring
    float height = fract(vWorldPos.y * 0.05 + time * 0.01);
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
'''
}

for path, content in shaders.items():
    full_path = os.path.join(r'c:/AbyssEngine', path)
    with open(full_path, 'w') as f:
        f.write(content)
    print(f'Updated {path}')

print('Shaders updated!')