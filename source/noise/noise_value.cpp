// Value noise implementation

#include <random>
#include "noise.h"

namespace Noise {

class ValueNoise {
public:
    ValueNoise(unsigned int seed = 0) : rng(seed) {
        for (int i = 0; i < 256; i++) perm[i] = i;
        for (int i = 0; i < 256; i++) {
            int target = (int)(rng() % 256);
            int tmp = perm[i]; perm[i] = perm[target]; perm[target] = tmp;
        }
        for (int i = 0; i < 256; i++) {
            perm[256 + i] = perm[i];
        }
    }

    float noise(float x, float y, float z) const {
        int xi = (int)std::floor(x) & 255;
        int yi = (int)std::floor(y) & 255;
        int zi = (int)std::floor(z) & 255;

        float x0 = x - (int)std::floor(x);
        float y0 = y - (int)std::floor(y);
        float z0 = z - (int)std::floor(z);

        float u = ufade(x0);
        float v = vfade(y0);
        float w = wfade(z0);

        int a = perm[perm[xi] + yi];
        int b = perm[perm[xi+1] + yi];
        int c = perm[perm[xi] + yi+1];
        int d = perm[perm[xi+1] + yi+1];

        float p0 = grad(a, x0, y0, z0);
        float p1 = grad(b, x0-1.0f, y0, z0);
        float p2 = grad(c, x0, y0-1.0f, z0);
        float p3 = grad(d, x0-1.0f, y0-1.0f, z0);
        float p4 = grad(a, x0, y0, z0-1.0f);
        float p5 = grad(b, x0-1.0f, y0, z0-1.0f);
        float p6 = grad(c, x0, y0-1.0f, z0-1.0f);
        float p7 = grad(d, x0-1.0f, y0-1.0f, z0-1.0f);

        float k0 = lerp(lerp(p0, p2, v), lerp(p1, p3, v), u);
        float k1 = lerp(lerp(p4, p6, v), lerp(p5, p7, v), u);

        return lerp(k0, k1, w);
    }

private:
    std::mt19937 rng;
    std::array<int, 512> perm;

    float ufade(float t) const { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
    float vfade(float t) const { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
    float wfade(float t) const { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
    float lerp(float a, float b, float t) const { return a + t*(b-a); }

    float grad(int hash, float x, float y, float z) {
        const int h = hash & 15;
        const float u = (hash & 1) ? x : -x;
        const float v = (hash & 2) ? y : -y;
        const float w = (hash & 4) ? z : -z;
        return u + v + w;
    }
};

} // namespace Noise
