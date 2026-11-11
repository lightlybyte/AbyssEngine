// Main noise module - combined system with single seed

#include "noise.h"

namespace Noise {

class NoiseModule {
public:
    NoiseModule(unsigned int seed = 0) : seed(seed), perlin(seed), simplex(seed), value(seed), billow(seed), rigid(seed) {}

    float combine(float x, float y, float z,
                  float perlinWeight = 1.0f,
                  float simplexWeight = 1.0f,
                  float valueWeight = 1.0f,
                  float billowWeight = 1.0f,
                  float rigidWeight = 1.0f) const {
        float total = 0.0f;
        float totalWeight = 0.0f;
        total += perlin.noise(x, y, z) * perlinWeight;
        totalWeight += perlinWeight;
        total += simplex.noise(x, y, z) * simplexWeight;
        totalWeight += simplexWeight;
        total += value.noise(x, y, z) * valueWeight;
        totalWeight += valueWeight;
        total += billow.noise(x, y, z) * billowWeight;
        totalWeight += billowWeight;
        total += rigid.noise(x, y, z) * rigidWeight;
        totalWeight += rigidWeight;
        if (totalWeight > 0.0f) return total / totalWeight;
        return 0.0f;
    }

    float getPerlin(float x, float y, float z) const { return perlin.noise(x, y, z); }
    float getSimplex(float x, float y, float z) const { return simplex.noise(x, y, z); }
    float getValue(float x, float y, float z) const { return value.noise(x, y, z); }
    float getBillow(float x, float y, float z) const { return billow.noise(x, y, z); }
    float getRigid(float x, float y, float z) const { return rigid.noise(x, y, z); }

    void setOctaves(int o) { rigid.setOctaves(o); }
    void setPersistence(float p) { rigid.setPersistence(p); }

private:
    unsigned int seed;
    PerlinNoise perlin;
    SimplexNoise simplex;
    ValueNoise value;
    BillowNoise billow;
    RigidMultiFractal rigid;
};

class SingleSeedSystem {
public:
    SingleSeedSystem(unsigned int seed = 0) : seed(seed), module(seed) {}

    float getNoise(float x, float y, float z) const {
        return module.combine(x, y, z);
    }

    float getPerlin(float x, float y, float z) const { return module.getPerlin(x, y, z); }
    float getSimplex(float x, float y, float z) const { return module.getSimplex(x, y, z); }
    float getValue(float x, float y, float z) const { return module.getValue(x, y, z); }
    float getBillow(float x, float y, float z) const { return module.getBillow(x, y, z); }
    float getRigid(float x, float y, float z) const { return module.getRigid(x, y, z); }

    void setSeed(unsigned int s) { seed = s; module = NoiseModule(seed); }
    unsigned int getSeed() const { return seed; }

private:
    unsigned int seed;
    NoiseModule module;
};

class HeightMapGenerator {
public:
    HeightMapGenerator(unsigned int seed = 0, int width = 256, int height = 256)
        : seed(seed), width(width), height(height), module(seed) {}

    void generateMap(std::vector<float>& heights) {
        heights.resize(width * height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float nx = (float)x / (width - 1) * 2.0f - 1.0f;
                float ny = (float)y / (height - 1) * 2.0f - 1.0f;
                float n = module.combine(nx, ny, 0.0f);
                heights[y * width + x] = (n + 1.0f) / 2.0f;
            }
        }
    }

    float getNoise(float x, float y) const { return module.combine(x, y, 0.0f); }

    void setSeed(unsigned int s) { seed = s; module = NoiseModule(seed); }
    unsigned int getSeed() const { return seed; }

private:
    unsigned int seed;
    int width, height;
    NoiseModule module;
};

} // namespace Noise
