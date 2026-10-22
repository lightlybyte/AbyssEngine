#pragma once

#include "glm/glm.hpp"
#include <cstdint>
#include <string>
#include <vector>

struct GLFWwindow;
struct VkRendererImpl;

struct VkVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

class VkRenderer {
public:
    VkRenderer();
    ~VkRenderer();

    VkRenderer(const VkRenderer&) = delete;
    VkRenderer& operator=(const VkRenderer&) = delete;

    // Init now takes the GLFWwindow directly so ImGui can hook into it
    bool init(GLFWwindow* window, uint32_t width, uint32_t height);

    // These are stubs for now, as the Hub doesn't render 3D yet
    uint32_t createMesh(const std::vector<VkVertex>& vertices, const std::vector<uint32_t>& indices);
    uint32_t createTexture(const unsigned char* rgbaPixels, int width, int height);
    void setDrawItem(uint32_t meshId, uint32_t textureId);
    void clearDrawItems();

    // This now renders the ImGui frame
    void drawFrame();

    bool shouldClose() const;
    void pollEvents();
    void waitIdle();
    double timeSeconds() const;

private:
    VkRendererImpl* impl = nullptr;
};