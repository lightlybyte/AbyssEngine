#include "vkrender.h"
#include "Terrain.h"
#include "Frustum.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

static std::vector<unsigned char> MakeCheckerTexture(int size, int squares) {
    std::vector<unsigned char> px((size_t)size * size * 4);
    const int cell = size / squares;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int cx = x / cell, cy = y / cell;
            const unsigned char v = (((cx + cy) & 1) == 0) ? 60 : 180;
            const size_t i = ((size_t)y * size + x) * 4;
            px[i+0] = v; px[i+1] = v; px[i+2] = v; px[i+3] = 255;
        }
    }
    return px;
}

int main() {
    try {
        VkRenderer renderer;
        if (!renderer.init("DATS - Terrain", 1280, 720)) {
            std::cerr << "Renderer init failed\n";
            return EXIT_FAILURE;
        }

        std::cout << "[1] Building terrain...\n";

        const int   CHUNKS = 8;
        const int   GRID_N = 32;
        const float WORLD  = 2000.0f;

        auto chunks = Terrain::BuildAll(renderer, CHUNKS, GRID_N, WORLD);

        std::cout << "[2] Chunks built: " << chunks.size() << "\n";

        auto checker = MakeCheckerTexture(256, 64);
        const uint32_t texId = renderer.createTexture(checker.data(), 256, 256);

        for (auto& c : chunks) {
            renderer.setDrawItem(c.meshId, texId);
        }

        const float aspect = 1280.0f / 720.0f;
        glm::mat4 projCull = glm::perspective(glm::radians(70.0f), aspect, 0.5f, 5000.0f);
        glm::mat4 projRender = projCull;
        projRender[1][1] *= -1.0f;

        const glm::vec3 up(0.0f, 1.0f, 0.0f);

        double lastPrint = 0.0;
        int frameCount = 0;

        std::cout << "[3] Entering main loop...\n";

        while (!renderer.shouldClose()) {
            renderer.pollEvents();

            const double t = renderer.timeSeconds();
            const float angle = (float)t * 0.2f;
            constexpr float orbitRadius = 250.0f;

            glm::vec3 eye(
                std::sin(angle) * orbitRadius,
                90.0f,
                std::cos(angle) * orbitRadius);
            glm::vec3 target(0.0f, 0.0f, 0.0f);
            glm::mat4 view = glm::lookAt(eye, target, up);

            glm::mat4 viewProjCull = projCull * view;
            Frustum frustum;
            frustum.Extract(viewProjCull);

            int drawn = 0;
            for (auto& c : chunks) {
                AABB box{ c.boxMin, c.boxMax };
                if (frustum.AABBInside(box)) ++drawn;
            }

            renderer.drawFrame(view, projRender);

            ++frameCount;
            const double now = renderer.timeSeconds();
            if (now - lastPrint >= 0.5) {
                const double fps = frameCount / (now - lastPrint);
                std::cout << "\rFPS: " << (int)fps
                          << "  drawn: " << drawn << "/" << chunks.size()
                          << "        " << std::flush;
                frameCount = 0;
                lastPrint = now;
            }
        }

        std::cout << "\n[4] Shutting down...\n";
        renderer.waitIdle();
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "FATAL: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}