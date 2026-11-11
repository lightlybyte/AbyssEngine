// Noise system for the Abyss Engine.
// Provides multiple noise functions with a single seed system.
namespace Noise {

class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 0);
    float noise(float x, float y) const;
    float noise(float x, float y, float z) const;
    float turbulence(float x, float y, float z, int octaves = 4, float persistence = 0.5f) const;
private:
    std::mt19937 rng;
    std::array<int, 256> perm;
    std::array<std::array<float, 3>, 256> gradients;
    float fade(float t) const;
    float lerp(float a, float b, float t) const;
};

class SimplexNoise {
public:
    SimplexNoise(unsigned int seed = 0);
    float noise(float wx, float wy, float wz) const;
    float turbulence(float x, float y, float z, int octaves = 4, float persistence = 0.5f) const;
private:
    int fastfloor(float x) const;
    std::mt19937 rng;
    std::array<int, 512> perm;
    std::array<std::array<float, 3>, 512> gradP;
};

class ValueNoise {
public:
    ValueNoise(unsigned int seed = 0);
    float noise(float x, float y, float z) const;
private:
    std::mt19937 rng;
    std::array<int, 512> perm;
    float ufade(float t) const;
    float vfade(float t) const;
    float wfade(float t) const;
    float lerp(float a, float b, float t) const;
    float grad(int hash, float x, float y, float z);
};

class BillowNoise {
public:
    BillowNoise(unsigned int seed = 0);
    float noise(float x, float y, float z) const;
    float turbulence(float x, float y, float z, int octaves = 4, float persistence = 0.5f) const;
private:
    SimplexNoise noise;
};

class RigidMultiFractal {
public:
    RigidMultiFractal(unsigned int seed = 0);
    float noise(float x, float y, float z) const;
    void setOctaves(int o);
    void setPersistence(float p);
private:
    SimplexNoise noise;
    int octaves;
    float persistence;
};

class NoiseModule {
public:
    NoiseModule(unsigned int seed = 0);
    float combine(float x, float y, float z,
                  float perlinWeight = 1.0f,
                  float simplexWeight = 1.0f,
                  float valueWeight = 1.0f,
                  float billowWeight = 1.0f,
                  float rigidWeight = 1.0f) const;
    float getPerlin(float x, float y, float z) const;
    float getSimplex(float x, float y, float z) const;
    float getValue(float x, float y, float z) const;
    float getBillow(float x, float y, float z) const;
    float getRigid(float x, float y, float z) const;
    void setOctaves(int o);
    void setPersistence(float p);
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
    SingleSeedSystem(unsigned int seed = 0);
    float getNoise(float x, float y, float z) const;
    float getPerlin(float x, float y, float z) const;
    float getSimplex(float x, float y, float z) const;
    float getValue(float x, float y, float z) const;
    float getBillow(float x, float y, float z) const;
    float getRigid(float x, float y, float z) const;
    void setSeed(unsigned int s);
    unsigned int getSeed() const;
private:
    unsigned int seed;
    NoiseModule module;
};

class HeightMapGenerator {
public:
    HeightMapGenerator(unsigned int seed = 0, int width = 256, int height = 256);
    void generateMap(std::vector<float>& heights);
    float getNoise(float x, float y) const;
    void setSeed(unsigned int s);
    unsigned int getSeed() const;
private:
    unsigned int seed;
    int width, height;
    NoiseModule module;
};

} // namespace Noise

