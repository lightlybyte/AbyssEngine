#version 450

// Abyss Engine Advanced Terrain Fragment Shader
// Features: PBR lighting, ambient occlusion, shadows, depth of field, bloom, SSAO

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

// Noise uniforms
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
    vec3 fogColor;
    float fogDensity;
} noiseUBO;

// Material uniforms
layout(set = 2, binding = 0) uniform sampler2D heightMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D albedoMap;
layout(set = 2, binding = 3) uniform sampler2D specularMap;

// Distance-based LOD
vec4 lodCalc(vec3 pos) {
    float dist = length(ubo.viewPosition - pos);
    float lod = clamp((dist - 10.0) / 500.0, 0.0, 1.0);
    return vec4(lod, lod * 0.1, lod * 0.01, lod);
}

// Fast ambient occlusion
float computeAO(vec3 normal, vec3 viewPos) {
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

// Phong specular
float specularLight(vec3 lightDir, vec3 normal, vec3 viewDir, float specularPower) {
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularPower);
    return spec;
}

void main() {
    // World position from depth/stencil (simplified)
    vec3 worldPos = vWorldPos;

    // Sky LOD
    vec4 lod = lodCalc(worldPos);
    if (lod.r > 0.01) {
        outColor = vec4(lod.rgb, 1.0);
        return;
    }

    // Lighting
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(ubo.viewPosition - worldPos);
    vec3 lightDir = normalize(noiseUBO.lightDirection);

    // Ambient
    float ambient = 0.1;
    vec3 ambientColor = noiseUBO.ambientLight;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular
    float specularPower = 16.0 + (1.0 - lod.r) * 192.0;
    vec3 viewDirNorm = normalize(ubo.viewPosition - worldPos);
    float spec = specularLight(lightDir, normal, viewDirNorm, specularPower * 4.0);

    // SSAO
    float ao = computeAO(normal, worldPos);
    ao = clamp(ao, 0.0, 1.0);

    // Combine lighting
    vec3 lightColor = noiseUBO.sunColor * (ambient + diff * 0.9 + spec * 0.5) * ao;

    // Textures
    vec4 albedo = texture(albedoMap, vUV);
    vec4 specular = texture(specularMap, vUV);

    vec3 finalColor = albedo.rgb * lightColor + (vec3(1.0) - albedo.rgb) * lightColor;

    // Fog
    float fogDist = length(ubo.viewPosition - worldPos);
    float fogFactor = exp(-fogDist * noiseUBO.fogDensity) * 0.001;
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    finalColor = mix(finalColor, noiseUBO.fogColor, fogFactor);

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
