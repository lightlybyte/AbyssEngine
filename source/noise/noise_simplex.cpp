// Simplex noise implementation (higher quality than Perlin)

#include <random>
#include "noise.h"

namespace Noise {

class SimplexNoise {
public:
    SimplexNoise(unsigned int seed = 0) : rng(seed) {
        // Initialize permutation and gradients
        for (int i = 0; i < 256; i++) {
            perm[i] = i;
            gradP[i] = {0.0f, 0.0f, 0.0f};
        }
        for (int i = 0; i < 256; i++) {
            int target = (int)(rng() % 256);
            int tmp = perm[i]; perm[i] = perm[target]; perm[target] = tmp;
        }
        // Gradients for simplex noise
        const int* p = perm.data();
        for (int i = 0; i < 256; i++) {
            int a = p[i] & 3;
            gradP[i] = { (a&1) ? 1.0f : -1.0f, (a&2) ? 1.0f : -1.0f, 0.0f };
        }
        // Mirror into 512
        for (int i = 0; i < 256; i++) {
            gradP[256 + i] = gradP[i];
            perm[256 + i] = perm[i];
        }
    }

    float noise(float wx, float wy, float wz) const {
        const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;

        float n = (wx + wy + wz) * F2;
        float n_x = wx + n;
        float n_y = wy + n;
        float n_z = wz + n;

        int i = fastfloor(n_x);
        int j = fastfloor(n_y);
        int k = fastfloor(n_z);

        float x0 = wx - (i + G2 * (i + j + k));
        float y0 = wy - (j + G2 * (i + j + k));
        float z0 = wz - (k + G2 * (i + j + k));

        int i1, j1, k1;
        int i2, j2, k2;

        if (x0 >= y0) {
            i1 = 1; j1 = 0; k1 = 0;
            i2 = 1; j2 = 1; k2 = 0;
        } else {
            i1 = 0; j1 = 1; k1 = 0;
            i2 = 1; j2 = 0; k2 = 1;
        }

        if (x0 < y0) {
            i1 = 0; j1 = 1; k1 = 0;
            i2 = 1; j2 = 1; k2 = 0;
        } else {
            i1 = 1; j1 = 1; k1 = 0;
            i2 = 0; j2 = 0; k2 = 1;
        }

        float x1 = x0 - G2 * i;
        float y1 = y0 - G2 * j;
        float z1 = z0 - G2 * k;

        float x2 = x0 - 1.0f + G2 * (i + j + k);
        float y2 = y0 - 1.0f + G2 * (i + j + k);
        float z2 = z0 - 1.0f + G2 * (i + j + k);

        int ii = i & 255;
        int jj = j & 255;
        int kk = k & 255;

        float t = 0.6f - (x0*x0 + y0*y0 + z0*z0);
        float n = 0.0f;
        if (t > 0) {
            t *= t;
            const float* g0 = &gradP[ii + jj];
            const float* g1 = &gradP[ii + kk];
            const float* g2 = &gradP[jj + kk];
            n = t * t * t * (t * (g0[0]*x0 + g0[1]*y0 + g0[2]*z0) +
                             7.0f * (g1[0]*x1 + g1[1]*y1 + g1[2]*z1) +
                             7.0f * (g2[0]*x2 + g2[1]*y2 + g2[2]*z2) + 5.0f * (g0[0]*x2 + g0[1]*y2 + g0[2]*z2));
        }

        return 2.0f * n - 1.0f;
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
    int fastfloor(float x) const { return x > 0 ? (int)x : (int)(x + 1); }

    std::mt19937 rng;
    std::array<int, 512> perm;
    std::array<std::array<float, 3>, 512> gradP;
};

} // namespace Noise
