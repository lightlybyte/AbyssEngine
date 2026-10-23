#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

#include "vkrender.h"
#include "engine.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

int main() {
    try {
        std::cout << "[DATS] Booting..." << std::endl;

        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        g_window = glfwCreateWindow(mode->width, mode->height, "DATS", nullptr, nullptr);
        if (!g_window) throw std::runtime_error("Failed to create window");

        if (!g_renderer.init(g_window, mode->width, mode->height)) {
            throw std::runtime_error("Failed to init renderer");
        }

        Engine::Init(g_renderer);

        double lastTime = g_renderer.timeSeconds();

        while (!g_renderer.shouldClose()) {
            g_renderer.pollEvents();

            double now = g_renderer.timeSeconds();
            double dt = now - lastTime;
            lastTime = now;

            Engine::Update(g_renderer, dt);

            switch (Engine::GetState()) {
                case GameState::MENU:
                    Engine::RenderMenu(g_renderer, g_renderer.ctx());
                    g_renderer.drawFrame();
                    break;
                case GameState::PLAYING:
                    Engine::RenderGame(g_renderer);
                    break;
                case GameState::PAUSED:
                    break;
            }
        }

        Engine::Shutdown();
        g_renderer.waitIdle();
        glfwDestroyWindow(g_window);
        glfwTerminate();

    } catch (const std::exception& e) {
        std::cerr << "\n[DATS] FATAL: " << e.what() << std::endl;
        if (g_window) {
            glfwDestroyWindow(g_window);
            glfwTerminate();
        }
    }

    std::cout << "\nPress Enter to Continue...";
    std::cin.get();
    return 0;
}