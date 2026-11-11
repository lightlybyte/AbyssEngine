#version 450

// Abyss Engine Advanced Lighting Vertex Shader
// Features: skinning, LOD, terrain displacement, optimized vertex processing

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;
layout(location = 4) in mat4 aBoneMatrices[10];
layout(location = 5) in int aBoneIndices[10];
layout(location = 6) in float aWeights[10];

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec4 vColor;

// Uniforms
layout(set = 0, binding = 0) uniform UBO_Matrices {
    mat4 modelViewProjection;
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 viewPosition;
    vec4 viewDirection;
    float time;
} ubo;

// Animation uniforms
layout(set = 1, binding = 0) uniform AnimationUniforms {
    float frameTime;
    float speed;
    int frames;
    mat4 skeletalBones[128];
} animUBO;

// Terrain displacement
layout(set = 2, binding = 0) uniform sampler2D heightMap;

// Constants
#define MAX_LIGHTS 4

void main() {
    // Skinned vertex
    mat4 boneMat = mat4(0.0);
    for(int i = 0; i < 10; i++) {
        if(aBoneIndices[i] < 128) {
            boneMat += aBoneMatrices[i] * aWeights[i];
        }
    }

    vec4 skinnedPos = boneMat * vec4(aPosition, 1.0);
    vec3 worldPos = (ubo.modelMatrix * skinnedPos).xyz;

    // Animation blend
    float frame = mod(ubo.frameTime * ubo.speed, float(animUBO.frames));
    worldPos.y += sin(frame * 0.5) * 0.05;

    // Height map displacement
    vec3 texPos = vec3(worldPos.x, worldPos.z) * 0.1;
    vec3 heightSample = texture(heightMap, texPos * 0.5 + 0.5).rgb;
    float heightDisplacement = heightSample.r * 0.5 + sin(texPos.x * 10.0 + ubo.time) * 0.02;
    worldPos.y += heightDisplacement;

    // Vertex shader noise
    vec3 noiseSample = vec3(worldPos.x * 0.1, worldPos.y * 0.1, worldPos.z * 0.1);
    float noiseVal = pnoise(noiseSample, vec3(10.0)) * 0.1;
    worldPos.y += noiseVal;

    // Compute normals
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
    vColor = aColor;
}

// Noise functions for vertex displacement
float hash(vec2 p) {
    p = fract(p * 1000.0);
    p = p * p + 41.0;
    p = p * p + 41.0;
    return fract(p.x * p.y);
}

float pnoise(vec3 p, vec3_scale) {
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

// Simplex noise
float snoise(vec3 p) {
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
    return nn / 2.0;
}
