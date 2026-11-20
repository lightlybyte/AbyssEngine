// Realistic terrain shader with earth-like coloring and atmospheric scattering
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
