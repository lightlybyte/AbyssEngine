#version 450

// Abyss Engine Advanced Lighting Fragment Shader
// Features: PBR material, normal mapping, SSAO, ambient occlusion, reflections

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

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

// Material textures
layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D specularMap;
layout(set = 2, binding = 3) uniform sampler2D roughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;

// Noise uniforms
layout(set = 3, binding = 0) uniform NoiseUniforms {
    uint seed;
    float scale;
    float heightScale;
    int octaves;
    float persistence;
    float amplitude;
    vec3 lightDirection;
    vec3 ambientLight;
    vec3 sunColor;
    vec3 fogColor;
    float fogDensity;
} noiseUBO;

// Lighting
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 direction;
    int type; // 0=point, 1=directional
};

// Material structure
struct Material {
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metalness;
    float ao;
};

// Compute material
Material getMaterial() {
    Material m;
    m.albedo = texture(albedoMap, vUV).rgb;
    m.roughness = texture(roughnessMap, vUV).r;
    m.metalness = texture(specularMap, vUV).r;
    m.ao = texture(aoMap, vUV).r;
    m.normal = normalize(texture(normalMap, vUV).rgb * 2.0 - 1.0);
    m.albedo = m.albedo * vColor.rgb;
    return m;
}

// Fragment lighting
vec3 calculateLighting(Material material, vec3 normal, vec3 viewDir, Light light) {
    vec3 result = vec3(0.0);
    vec3 lightDir = normalize(light.position - light.position);

    if (light.type == 0) {
        lightDir = normalize(-light.direction);
    } else {
        lightDir = normalize(light.direction);
    }

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    result += material.albedo * light.color * diff * light.intensity;

    // Specular
    vec3 halfVec = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVec), 0.0), material.roughness * 128.0);
    result += vec3(1.0) * light.color * spec * light.intensity * 0.5;

    // Ambient
    result += material.albedo * 0.1 * light.color;

    // Ambient occlusion
    result *= material.ao;

    return result;
}

// SSAO
float computeSSAO(vec3 normal, vec3 viewPos) {
    float ao = 0.0;
    for(int i = 0; i < 16; i++) {
        vec2 c = vec2(float(i & 3), float(i >> 2)) / 4.0;
        vec3 sample = normalize(vec3(hash(c), 1.0));
        vec3 rayDir = normalize(sample - normal);
        float t = 0.0;
        float p = 1.0;
        for(int j = 0; j < 5; j++) {
            vec3 point = viewPos + rayDir * t;
            float d = texture(heightMap, (point.xy * 0.1 + 0.5)).r * 2.0 - 1.0;
            if(d > 0.0) {
                p *= 0.5;
                break;
            }
            t += 0.05;
        }
        ao += p;
    }
    return ao / 16.0;
}

void main() {
    Material material = getMaterial();

    // Normal transform
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(ubo.viewPosition - vWorldPos);

    // Lighting
    vec3 lightColor = vec3(0.0);
    vec3 viewDirNorm = normalize(ubo.viewPosition - vWorldPos);

    Light sun;
    sun.direction = normalize(ubo.viewDirection.xyz);
    sun.intensity = 1.0;
    sun.color = vec3(1.0, 0.95, 0.85);
    sun.type = 1;

    lightColor += calculateLighting(material, normal, viewDir, sun);

    // SSAO
    float ao = computeSSAO(normal, vWorldPos);
    ao = clamp(ao, 0.0, 1.0);
    lightColor *= ao;

    // HDR tone mapping
    vec3 color = vec3(1.0) - exp(-lightColor * 0.5);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    // Bloom
    vec3 bloom = color * 0.2;
    color += bloom;

    outColor = vec4(color, 1.0);
}
