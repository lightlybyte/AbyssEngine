#include <iostream>
#include <stdexcept>

#include "GLFW/glfw3.h"

// Define Nuklear implementation in main.cpp
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#include "nuklear.h"

#include "vkrender.h"

static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

// Nuklear context
static struct nk_context* g_ctx = nullptr;

static void draw_hub_ui() {
    // 1. Start Nuklear Frame
    nk_glfw3_new_frame();

    // 2. Draw the UI
    if (nk_begin(g_ctx, "Abyss Engine Hub", nk_rect(100, 100, 800, 600),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(g_ctx, 30, 1);
        nk_label(g_ctx, "Abyss Engine v2027.Q1 (Endeavor)", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(g_ctx, 25, 1);
        nk_label(g_ctx, "No recent projects found in C:\\AbyssProjects", NK_TEXT_LEFT);

        nk_layout_row_dynamic(g_ctx, 40, 3);
        if (nk_button_label(g_ctx, "New Project")) {
            std::cout << "New Project clicked" << std::endl;
        }
        if (nk_button_label(g_ctx, "Open Project")) {
            std::cout << "Open Project clicked" << std::endl;
        }
        if (nk_button_label(g_ctx, "Quit")) {
            glfwSetWindowShouldClose(g_window, true);
        }
    }
    nk_end(g_ctx);
}

int main() {
    try {
        std::cout << "[Abyss Engine] Booting Hub..." << std::endl;

        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        
        g_window = glfwCreateWindow(mode->width, mode->height, "Abyss Engine Hub", nullptr, nullptr);
        if (!g_window) throw std::runtime_error("Failed to create window");

        // We get the Nuklear context from the renderer after it initializes
        if (!g_renderer.init(g_window, mode->width, mode->height)) {
            throw std::runtime_error("Failed to init renderer");
        }

        // In a real setup, you'd get the context from the renderer or initialize it here.
        // For this example, we assume g_ctx is accessible or we create a default one.
        // If VkRenderer::init creates it, you'd need a getter.
        // For simplicity, we'll init a default context here for the UI logic.
        g_ctx = nk_glfw3_init(g_window, nullptr, nullptr, 0, nullptr, 0, VK_FORMAT_UNDEFINED, 0, 0, 0); 
        // NOTE: This is a placeholder. You MUST match the init signature of your specific 
        // nuklear_glfw_vulkan.h file. It likely requires the device, physical device, etc.

        while (!g_renderer.shouldClose()) {
            g_renderer.pollEvents();

            draw_hub_ui();
            g_renderer.drawFrame();
        }

        g_renderer.waitIdle();
        glfwDestroyWindow(g_window);
        glfwTerminate();
        
    } catch (const std::exception& e) {
        std::cerr << "\n[Abyss Engine] FATAL: " << e.what() << std::endl;
        if (g_window) {
            glfwDestroyWindow(g_window);
            glfwTerminate();
        }
    }

    std::cout << "\nPress Enter to Continue...";
    std::cin.get();
    return 0;
}