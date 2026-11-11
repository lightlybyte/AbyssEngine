// Perlin noise implementation (2D and 3D)

#include <random>
#include <array>
#include "noise.h"

namespace Noise {

class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 0) : rng(seed) {
        // Initialize permutation table
        for (int i = 0; i < 256; i++) perm[i] = i;
        for (int i = 0; i < 256; i++) {
            int target = (int)(rng() % 256);
            int tmp = perm[i]; perm[i] = perm[target]; perm[target] = tmp;
        }
        // Generate gradients
        for (int i = 0; i < 256; i++) {
            int a = perm[i] & 3;
            gradients[i] = { (a&1) ? 1.0f : -1.0f, (a&2) ? 1.0f : -1.0f, 0.0f };
        }
    }

    float noise(float x, float y) const {
        int i = ((int)x) & 255;
        int j = ((int)y) & 255;
        float fx = x - (int)x;
        float fy = y - (int)y;
        float fx1 = fx - 1.0f;
        float fy1 = fy - 1.0f;

        const float* g00 = gradients[perm[perm[i]+0]];
        const float* g10 = gradients[perm[perm[i+1]+0]];
        const float* g01 = gradients[perm[perm[i]+1]];
        const float* g11 = gradients[perm[perm[i+1]+1]];

        float n0 = g00[0]*fx1 + g00[1]*fy;
        float n1 = g10[0]*fx + g10[1]*fy;
        float n2 = g01[0]*fx1 + g01[1]*fy1;
        float n3 = g11[0]*fx + g11[1]*fy1;

        float u = fade(fx), v = fade(fy);
        return lerp(lerp(n0, n2, u), lerp(n1, n3, u), v);
    }

    float noise(float x, float y, float z) const {
        int i = ((int)x) & 255;
        int j = ((int)y) & 255;
        int k = ((int)z) & 255;
        float fx = x - (int)x;
        float fy = y - (int)y;
        float fz = z - (int)z;
        float fx1 = fx - 1.0f;
        float fy1 = fy - 1.0f;
        float fz1 = fz - 1.0f;

        int a = perm[perm[perm[i]+j]+k];
        int b = perm[perm[perm[i+1]+j]+k];
        int c = perm[perm[perm[i]+j+1]+k];
        int d = perm[perm[perm[i+1]+j+1]+k];
        int e = perm[perm[perm[i]+j+1]+k+1];
        int f = perm[perm[perm[i+1]+j+1]+k+1];
        int g = perm[perm[perm[perm[i]+j+1]+k]+1];
        int h = perm[perm[perm[perm[i+1]+j]+k]+1];

        const float* g000 = gradients[a];
        const float* g100 = gradients[b];
        const float* g010 = gradients[c];
        const float* g110 = gradients[d];
        const float* g001 = gradients[e];
        const float* g101 = gradients[f];
        const float* g011 = gradients[g];
        const float* g111 = gradients[h];

        float n000 = g000[0]*fx1 + g000[1]*fy + g000[2]*fz;
        float n100 = g100[0]*fx + g100[1]*fy + g100[2]*fz;
        float n010 = g010[0]*fx1 + g010[1]*fy1 + g010[2]*fz;
        float n110 = g110[0]*fx + g110[1]*fy1 + g110[2]*fz;
        float n001 = g001[0]*fx1 + g001[1]*fy + g001[2]*fz1;
        float n101 = g101[0]*fx + g101[1]*fy + g101[2]*fz1;
        float n011 = g011[0]*fx1 + g011[1]*fy1 + g011[2]*fz1;
        float n111 = g111[0]*fx + g111[1]*fy1 + g111[2]*fz1;

        float u = fade(fx), v = fade(fy), w = fade(fz);
        return lerp(
            lerp(lerp(n000, n100, u), lerp(n010, n110, u), v),
            lerp(lerp(n001, n101, u), lerp(n011, n111, u), v), w);
    }

    float turbulence(float x, float y, float z, int octaves = 4, float persistence = 0.5f) const {
        float total = 0.0f;
        float amp = 1.0f;
        float freq = 1.0f;
        for (int i = 0; i < octaves; i++) {
            total += noise(x*freq, y*freq, z*freq) * amp;
            amp *= persistence;
            freq *= 2.0f;
        }
        return total;
    }

private:
    std::mt19937 rng;
    std::array<int, 256> perm;
    std::array<std::array<float, 3>, 256> gradients;

    float fade(float t) const { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
    float lerp(float a, float b, float t) const { return a + t*(b-a); }
};

} // namespace Noise
