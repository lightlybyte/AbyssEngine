// Billow and Rigid Multi-fractal noise

#include "noise.h"

namespace Noise {

class BillowNoise {
public:
    BillowNoise(unsigned int seed = 0) : noise(seed) {}

    float noise(float x, float y, float z) const {
        float v = noise.noise(x, y, z);
        return std::abs(v) * 2.0f - 1.0f;
    }

    float turbulence(float x, float y, float z, int octaves = 4, float persistence = 0.5f) const {
        float total = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        for (int i = 0; i < octaves; i++) {
            total += std::abs(noise.noise(x*freq, y*freq, z*freq)) * amp;
            amp *= persistence;
            freq *= 2.0f;
        }
        return total;
    }

private:
    SimplexNoise noise;
};

class RigidMultiFractal {
public:
    RigidMultiFractal(unsigned int seed = 0) : noise(seed), octaves(6), persistence(0.5f) {}

    float noise(float x, float y, float z) const {
        float value = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        for (int i = 0; i < octaves; i++) {
            value = std::abs(value) * 2.0f + noise.noise(x*freq, y*freq, z*freq);
            amp *= persistence;
            freq *= 2.0f;
        }
        return value * amp;
    }

    void setOctaves(int o) { octaves = o; }
    void setPersistence(float p) { persistence = p; }

private:
    SimplexNoise noise;
    int octaves;
    float persistence;
};

} // namespace Noise
