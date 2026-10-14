#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

// Vertex format the renderer expects
struct VkVertex;

// A single terrain chunk
struct TerrainChunk {
    uint32_t meshId = 0;
    glm::vec3 boxMin{0};
    glm::vec3 boxMax{0};
    glm::vec3 center{0};
    float radius = 0.0f;
};

// Terrain generation and management
namespace Terrain {

    // Height function — replace with simplex later
    float HeightAt(float x, float z);

    // Build a single chunk into verts/indices
    void BuildChunk(std::vector<VkVertex>& verts,
                    std::vector<uint32_t>& indices,
                    float originX, float originZ,
                    float size, int gridN);

    // Build all chunks and upload to renderer
    // Returns the list of chunks with their mesh IDs
    std::vector<TerrainChunk> BuildAll(class VkRenderer& renderer,
                                       int chunks,
                                       int gridN,
                                       float world);
}