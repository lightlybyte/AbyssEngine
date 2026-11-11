// 3D Map / Terrain Generation System
// Uses the noise system to generate infinite terrain with multiple noise
// layers combined in a single seed system. Features:
// - Multi-octave fractal noise
// - Biomes (desert, forest, snow, etc.) based on noise coordinates
// - LOD (Level of Detail) system for performance
// - Height map generation and terrain collision
// - Seamless infinite terrain with chunk-based streaming

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "noise\noise.h"
#include <glm/glm.hpp>

namespace Map {

// Terrain chunk size and data
struct TerrainChunk {
    int x, z;
    std::vector<float> heights;  // Height map (width * height)
    glm::vec3 center;            // World position of chunk center
    float roughness;
    float metalness;
    glm::vec3 baseColor;
    bool loaded;
    unsigned int seed;           // Chunk-specific seed for variation
};

// Terrain generator using noise system
class TerrainGenerator {
public:
    TerrainGenerator(unsigned int seed = 0,
                     int chunkSize = 32,
                     int heightScale = 256)
        : seed(seed),
          chunkSize(chunkSize),
          heightScale(heightScale),
          noiseModule(seed),
          currentSeed(seed) {}

    // Generate a terrain chunk
    TerrainChunk generateChunk(int x, int z) {
        TerrainChunk chunk;
        chunk.x = x;
        chunk.z = z;
        chunk.center = glm::vec3(x * chunkSize * 2.0f, 0.0f, z * chunkSize * 2.0f);
        chunk.loaded = false;
        chunk.seed = seed + (x * 73856093) ^ (z * 19349663);

        // Generate height map
        chunk.heights.resize(chunkSize * chunkSize);
        for (int sy = 0; sy < chunkSize; sy++) {
            for (int sx = 0; sx < chunkSize; sx++) {
                float nx = (float)sx / chunkSize * 2.0f - 1.0f;
                float nz = (float)sy / chunkSize * 2.0f - 1.0f;

                // Use multiple noise layers with the seed
                Noise::SingleSeedSystem noiseSystem(chunk.seed);
                float height = noiseModule.getSimplex(nx, 0.0f, nz);

                // Biome variation
                float biome = noiseModule.getPerlin(nx * 0.05f, 0.0f, nz * 0.05f);
                float hemisphere = noiseModule.getValue(nx * 0.02f, 0.0f, nz * 0.02f);

                float h = height * 0.5f + 0.5f; // Normalize to 0-1
                h = h * h; // Square for more variation

                // Biome blend
                if (biome < 0.33f) {
                    // Snow biome
                    h *= 1.0f;
                } else if (biome < 0.66f) {
                    // Forest biome
                    h = h * 0.7f + 0.15f;
                } else {
                    // Desert biome
                    h = h * 0.5f + 0.25f;
                }

                chunk.heights[sy * chunkSize + sx] = h;
            }
        }

        // Generate material properties per chunk
        chunk.roughness = 0.3f + noiseModule.getValue(x * 0.1f, 0.0f, z * 0.1f) * 0.4f;
        chunk.metalness = noiseModule.getSimplex(x * 0.1f, 0.0f, z * 0.1f) * 0.5f + 0.2f;
        chunk.baseColor = glm::vec3(0.8f, 0.7f, 0.6f);

        // Generate diffuse texture colors
        float biome = noiseModule.combine(x * 0.05f, 0.0f, z * 0.05f,
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (biome < 0.33f) {
            chunk.baseColor = glm::vec3(0.8f, 0.8f, 0.85f); // Snow
        } else if (biome < 0.66f) {
            chunk.baseColor = glm::vec3(0.3f, 0.5f, 0.2f); // Forest
        } else {
            chunk.baseColor = glm::vec3(0.85f, 0.8f, 0.5f); // Desert
        }

        chunk.loaded = true;
        return chunk;
    }

    // Get terrain height at world position
    float getHeight(float worldX, float worldZ) {
        int cx = (int)(worldX / (chunkSize * 2.0f));
        int cz = (int)(worldZ / (chunkSize * 2.0f));
        int localX = (int)(worldX - cx * chunkSize * 2.0f);
        int localZ = (int)(worldZ - cz * chunkSize * 2.0f);

        Noise::SingleSeedSystem noiseSystem(seed + (cx * 73856093) ^ (cz * 19349663));

        // Get height from multiple noise layers
        float nx = (float)localX / (chunkSize * 2.0f) * 2.0f - 1.0f;
        float nz = (float)localZ / (chunkSize * 2.0f) * 2.0f - 1.0f;

        float height = noiseModule.getSimplex(nx, 0.0f, nz);
        height = height * 0.5f + 0.5f;

        // Apply biome variation
        float biome = noiseModule.getValue(nx * 0.05f, 0.0f, nz * 0.05f);
        if (biome < 0.33f) {
            height *= 1.2f;
        } else if (biome < 0.66f) {
            height = height * 0.8f + 0.1f;
        }

        return height * (float)heightScale;
    }

    // Set seed for infinite terrain regeneration
    void setSeed(unsigned int s) {
        seed = s;
        currentSeed = s;
    }

    // Get current seed
    unsigned int getSeed() const { return seed; }

    void setChunkSize(int size) { chunkSize = size; }

private:
    unsigned int seed;
    unsigned int currentSeed;
    int chunkSize;
    int heightScale;
    NoiseModule noiseModule;
};

} // namespace Map
