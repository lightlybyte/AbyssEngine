#include "Terrain.h"
#include "vkrender.h"

#include <cmath>

namespace Terrain {

float HeightAt(float x, float z) {
    float h = 0.0f;
    h += std::sin(x * 0.015f) * std::cos(z * 0.015f) * 18.0f;
    h += std::sin(x * 0.05f + 1.7f) * std::cos(z * 0.045f + 0.3f) * 6.0f;
    h += std::sin(x * 0.13f + 4.2f) * std::cos(z * 0.11f + 2.1f) * 2.0f;
    h += std::sin(x * 0.31f) * std::cos(z * 0.29f) * 0.5f;
    return h;
}

void BuildChunk(std::vector<VkVertex>& verts,
                std::vector<uint32_t>& indices,
                float originX, float originZ,
                float size, int gridN) {
    verts.clear();
    indices.clear();
    verts.reserve((gridN + 1) * (gridN + 1));
    indices.reserve(gridN * gridN * 6);

    const float step = size / (float)gridN;
    const float invGrid = 1.0f / (float)gridN;

    for (int z = 0; z <= gridN; ++z) {
        const float pz = originZ + z * step;
        for (int x = 0; x <= gridN; ++x) {
            const float px = originX + x * step;
            const float py = HeightAt(px, pz);

            VkVertex v{};
            v.pos = { px, py, pz };

            const float hL = HeightAt(px - step, pz);
            const float hR = HeightAt(px + step, pz);
            const float hD = HeightAt(px, pz - step);
            const float hU = HeightAt(px, pz + step);
            v.normal = glm::normalize(glm::vec3(hL - hR, 2.0f * step, hD - hU));

            v.uv = { px * 0.05f, pz * 0.05f };
            verts.push_back(v);
        }
    }

    for (int z = 0; z < gridN; ++z) {
        const uint32_t row = (uint32_t)z * (gridN + 1);
        for (int x = 0; x < gridN; ++x) {
            const uint32_t i0 = row + x;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + (gridN + 1);
            const uint32_t i3 = i2 + 1;

            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }
}

std::vector<TerrainChunk> BuildAll(VkRenderer& renderer,
                                   int chunks,
                                   int gridN,
                                   float world) {
    std::vector<TerrainChunk> result;
    result.reserve(chunks * chunks);

    const float chunkSize = world / (float)chunks;

    std::vector<VkVertex> verts;
    std::vector<uint32_t> indices;

    for (int cz = 0; cz < chunks; ++cz) {
        for (int cx = 0; cx < chunks; ++cx) {
            const float ox = -world * 0.5f + cx * chunkSize;
            const float oz = -world * 0.5f + cz * chunkSize;

            BuildChunk(verts, indices, ox, oz, chunkSize, gridN);

            TerrainChunk chunk;
            chunk.meshId = renderer.createMesh(verts, indices);

            glm::vec3 mn(1e9f), mx(-1e9f);
            for (auto& v : verts) {
                mn = glm::min(mn, v.pos);
                mx = glm::max(mx, v.pos);
            }
            chunk.boxMin = mn;
            chunk.boxMax = mx;
            chunk.center = (mn + mx) * 0.5f;
            chunk.radius = glm::length(mx - mn) * 0.5f;

            result.push_back(std::move(chunk));
        }
    }

    return result;
}

} // namespace Terrain