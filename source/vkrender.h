#pragma once

#include <glm/glm.hpp>
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

    bool init(GLFWwindow* window, uint32_t width, uint32_t height);

    uint32_t createMesh(const std::vector<VkVertex>& vertices, const std::vector<uint32_t>& indices);
    uint32_t createTexture(const unsigned char* rgbaPixels, int width, int height);
    void setDrawItem(uint32_t meshId, uint32_t textureId);
    void clearDrawItems();

    void drawFrame();
    void drawGameFrame();

    bool shouldClose() const;
    void pollEvents();
    void waitIdle();
    double timeSeconds() const;

    struct nk_context* ctx() const;

private:
    VkRendererImpl* impl = nullptr;
};