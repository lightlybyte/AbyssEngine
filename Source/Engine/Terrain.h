#pragma once

#include "vkrender.h"

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

struct TerrainChunk {
    uint32_t meshId = 0;
    glm::vec3 boxMin{0};
    glm::vec3 boxMax{0};
    glm::vec3 center{0};
    float radius = 0.0f;
};

namespace Terrain {

    float HeightAt(float x, float z);

    void BuildChunk(std::vector<VkVertex>& verts,
                    std::vector<uint32_t>& indices,
                    float originX, float originZ,
                    float size, int gridN);

    std::vector<TerrainChunk> BuildAll(VkRenderer& renderer,
                                       int chunks,
                                       int gridN,
                                       float world);
}