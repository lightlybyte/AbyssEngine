#version 450

// Abyss Engine Animation Vertex Shader
// Features: skeletal animation, morph targets, LOD culling

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in mat4 aBoneMatrices[10]; // up to 10 bones
layout(location = 4) in int aBoneIndices[10];   // bone indices
layout(location = 5) in float aWeights[10];     // blending weights

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
    float time;
} ubo;

// Animation frame and bone weights
layout(set = 1, binding = 0) uniform AnimationUniforms {
    float frameTime;
    float speed;
    int frames;
    mat4 skeletalBones[128]; // up to 128 bones
} animUBO;

layout(set = 2, binding = 0) uniform sampler2D heightMap;

// Vertex shader noise
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

// Combined terrain noise for vertex displacement
vec3 animatedNoise(vec3 p) {
    float perlin = pnoise(p * 0.1, 10.0);
    float simplex = snoise(p * 0.1);
    float value = psnoise(p * 0.1);
    float billow = abs(pnoise(p * 0.15)) * 2.0 - 1.0;
    float rigid = abs(psnoise(p * 0.12)) * 2.0 - 1.0;
    return (perlin * 0.3 + simplex * 0.3 + value * 0.2 + billow * 0.1 + rigid * 0.1);
}

void main() {
    // Bone matrix calculation
    mat4 boneMat = mat4(0.0);
    for(int i = 0; i < 10; i++) {
        if(aBoneIndices[i] < 128) {
            boneMat += aBoneMatrices[i] * aWeights[i];
        }
    }

    // Skinned vertex
    vec4 skinnedPos = boneMat * vec4(aPosition, 1.0);
    vec3 worldPos = (ubo.modelMatrix * skinnedPos).xyz;

    // Animation displacement
    float frame = mod(ubo.frameTime * ubo.speed, float(animUBO.frames));
    float animOffset = sin(frame * 0.5) * 0.05;
    worldPos.y += animOffset;

    // Height map displacement
    vec3 texPos = vec3(worldPos.x, worldPos.z) * 0.1;
    vec3 heightSample = texture(heightMap, texPos * 0.5 + 0.5).rgb;
    float heightDisplacement = heightSample.r * 0.5 + sin(texPos.x * 10.0 + ubo.time) * 0.02;
    worldPos.y += heightDisplacement;

    // Vertex shader noise
    float noiseVal = combinedNoise(worldPos);
    worldPos.y += noiseVal * 0.1;

    // Compute normals using finite differences
    vec3 pos1 = (ubo.modelMatrix * vec4(aPosition + vec3(0.001, 0, 0), 1.0)).xyz;
    vec3 pos2 = (ubo.modelMatrix * vec4(aPosition + vec3(0, 0.001, 0), 1.0)).xyz;
    vec3 pos3 = (ubo.modelMatrix * vec4(aPosition + vec3(0, 0, 0.001), 1.0)).xyz;
    vec3 normal = normalize(cross(pos2 - worldPos, pos3 - worldPos));
    normal = normalize(normal + cross(worldPos - pos1, worldPos - pos3));

    vec4 worldPos4 = vec4(worldPos, 1.0);
    gl_Position = ubo.modelViewProjection * worldPos4;

    vWorldPos = worldPos;
    vNormal = normalize(ubo.normalMatrix * normal);
    vUV = aUV;
    vHeight = worldPos.y;

    // LOD computation (simplified)
    float dist = length(ubo.viewPosition - worldPos);
    gl_PointSize = clamp(1.0 - dist * 0.001, 1.0, 100.0);
}
