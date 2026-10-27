#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vkrender.h"
#include "engine.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

// NOTE: Do NOT include "nuklear_glfw_vulkan.h" here. It causes linker conflicts.
// Forward declaration (implementation is in vkrender.cpp via NK_IMPLEMENTATION)
void nk_glfw3_new_frame();

static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

int main() {
    try {
        std::cout << "[DATS] Booting..." << std::endl;

        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        // Pass the monitor as the 4th argument to make it true fullscreen
        g_window = glfwCreateWindow(mode->width, mode->height, "DATS", monitor, nullptr);
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

            nk_glfw3_new_frame();

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