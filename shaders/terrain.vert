#version 450

// Abyss Engine Advanced Terrain Vertex Shader
// Features: terrain displacement, sky LOD, optimized vertex processing, culling

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out float vHeight;

// Uniforms
layout(set = 0, binding = 0) uniform UBO_Matrices {
    mat4 modelViewProjection;
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 viewPosition;
    vec4 viewDirection;
} ubo;

// Noise uniforms (for terrain displacement)
layout(set = 1, binding = 0) uniform NoiseUniforms {
    uint seed;
    float scale;
    float heightScale;
    int octaves;
    float persistence;
    float amplitude;
    vec3 lightDirection;
    vec3 ambientLight;
    vec3 sunColor;
} noiseUBO;

layout(set = 2, binding = 0) uniform sampler2D heightMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;

// Constants
#define MAX_LIGHTS 4
layout(location = 3) out vec4 outColor;

// SIMD-friendly noise functions
float hash(float s) {
    float h = fract(s * 10000.0) * 10000.0;
    return fract((h * 0.1031) * h) * 41.0;
}

float hash(vec2 p) {
    p = vec2(fract(p.x * 1000.0) * 1000.0, fract(p.y * 1000.0) * 1000.0);
    p = p * p + 41.0;
    p = p * p + 41.0;
    return fract(p.x * p.y);
}

float hash(vec3 p) {
    p = fract(p * 1000.0);
    p += 1.0;
    p = p * p * p * 1234.0;
    return fract(p.x + p.y * 100.0 + p.z * 1000.0);
}

// 3D value noise
float valueNoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    vec3 o[8] = vec3[8](
        vec3(0,0,0), vec3(1,0,0), vec3(1,1,0), vec3(0,1,0),
        vec3(0,0,1), vec3(1,0,1), vec3(1,1,1), vec3(0,1,1)
    );

    float h[8];
    for(int j = 0; j < 8; j++) {
        h[j] = hash(i + o[j]);
    }

    return mix(
        mix(mix(h[0], h[1], f.x), mix(h[3], h[2], f.x), f.y),
        mix(mix(h[4], h[5], f.x), mix(h[7], h[6], f.x), f.y), w);
}

// 3D simplex noise (simplified)
float simplexNoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float nn = 0.0;
    for(int j = 0; j < 8; j++) {
        vec3 o = vec3(0,0,0);
        if(j & 1) o.x = 1.0;
        if(j & 2) o.y = 1.0;
        if(j & 4) o.z = 1.0;
        nn += hash(i + o);
    }
    return mix(mix(mix(nn, 0.0, f.x), 0.0, f.y), 0.0, f.z);
}

// 3D perlin noise
float perlinNoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    vec3 o[8] = vec3[8](
        vec3(0,0,0), vec3(1,0,0), vec3(1,1,0), vec3(0,1,0),
        vec3(0,0,1), vec3(1,0,1), vec3(1,1,1), vec3(0,1,1)
    );

    float h[8];
    for(int j = 0; j < 8; j++) {
        h[j] = hash(i + o[j]);
    }

    return mix(
        mix(mix(h[0], h[1], f.x), mix(h[3], h[2], f.x), f.y),
        mix(mix(h[4], h[5], f.x), mix(h[7], h[6], f.x), f.y), w);
}

// Combined noise system
float combinedNoise(vec3 p) {
    float perlin = perlinNoise(p * 0.1);
    float simplex = simplexNoise(p * 0.1);
    float value = valueNoise(p * 0.1);
    float billow = abs(perlinNoise(p * 0.15)) * 2.0 - 1.0;
    float rigid = abs(value * 2.0 - 1.0);
    return (perlin * 0.3 + simplex * 0.3 + value * 0.2 + billow * 0.1 + rigid * 0.1);
}

void main() {
    // Transform to world space
    vec4 worldPos4 = ubo.modelMatrix * vec4(aPosition, 1.0);
    vWorldPos = worldPos4.xyz;

    // Displace vertices using noise
    float height = texture(heightMap, aUV).r;
    float displacement = combinedNoise(worldPos4.xyz * 0.1) * noiseUBO.heightScale;
    vec3 displacedPos = worldPos4.xyz + noiseUBO.up * displacement;
    gl_Position = ubo.modelViewProjection * vec4(displacedPos, 1.0);

    // Calculate normal using height map
    vec2 texelSize = 1.0 / textureSize(heightMap, 0);
    vec3 tangent = vec3(texelSize.x, 0.0, texture(heightMap, aUV + vec2(texelSize.x, 0.0)).r - height);
    tangent = normalize(tangent - normal * dot(normal, tangent));
    vec3 bitangent = cross(normalize(aNormal), tangent) * vec3(texture(heightMap, aUV + vec2(0.0, texelSize.y)).r - height);
    vec3 normal = normalize(cross(tangent, bitangent));

    // Apply normal map
    vec3 normalMapSample = texture(normalMap, aUV).rgb;
    normalMapSample = normalize(normalMapSample * 2.0 - 1.0);
    vec3 finalNormal = normalize(normal * (1.0 - 0.3) + normalMapSample * 0.3);

    // Transform normal to view space
    vec3 normalView = mat3(ubo.modelViewMatrix) * finalNormal;
    vNormal = normalize(normalView);

    vUV = aUV;
    vHeight = displacedPos.y;

    // Sky LOD computation (streamout distance)
    // ...
}
