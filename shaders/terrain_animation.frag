#version 450

// Abyss Engine Animation Fragment Shader
// Features: PBR material, normal mapping, specular, normal mapping

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in float vHeight;

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

// Material uniforms
layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D specularMap;
layout(set = 2, binding = 3) uniform sampler2D roughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;

layout(location = 0) out vec4 outColor;

// PBR Material
struct Material {
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metalness;
    float ao;
};

Material getMaterial() {
    Material m;
    vec4 albedo = texture(albedoMap, vUV);
    m.albedo = albedo.rgb;
    m.roughness = texture(roughnessMap, vUV).r;
    m.metalness = texture(specularMap, vUV).r;
    m.ao = texture(aoMap, vUV).r;
    m.normal = normalize(texture(normalMap, vUV).rgb * 2.0 - 1.0);
    return m;
}

// Deferred lighting
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 direction;
    int type; // 0=point, 1=directional
};

vec3 calculateLighting(Material material, vec3 normal, vec3 viewDir, Light light) {
    vec3 result = vec3(0.0);
    vec3 lightDir = light.position - light.position;

    if (light.type == 0) {
        lightDir = normalize(-light.direction);
    } else {
        lightDir = normalize(light.direction);
    }

    vec3 halfVec = normalize(lightDir + viewDir);

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    result += material.albedo * light.color * diff * light.intensity;

    // Specular
    float spec = pow(max(dot(normal, halfVec), 0.0), material.roughness * 128.0);
    result += vec3(1.0) * light.color * spec * light.intensity * 0.5;

    // Ambient
    result += material.albedo * 0.1 * light.color;

    // Ambient occlusion
    result *= material.ao;

    return result;
}

void main() {
    Material material = getMaterial();

    // Lighting
    vec3 lightColor = vec3(0.0);
    vec3 viewDir = normalize(ubo.viewPosition - vWorldPos);

    Light sun;
    sun.direction = normalize(ubo.viewDirection.xyz);
    sun.intensity = 1.0;
    sun.color = vec3(1.0, 0.95, 0.85); // Warm sunlight
    sun.type = 1;

    lightColor += calculateLighting(material, vNormal, viewDir, sun);

    // HDR tone mapping
    vec3 color = vec3(1.0) - exp(-lightColor * 0.5);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    // Bloom
    vec3 bloom = color * 0.2;
    color += bloom;

    outColor = vec4(color, 1.0);
}
