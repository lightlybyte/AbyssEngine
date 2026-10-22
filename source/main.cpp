#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include "GLFW/glfw3.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "vkrender.h"

static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

static void draw_hub_ui() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) glfwSetWindowShouldClose(g_window, true);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
    
    ImGui::Begin("Abyss Engine Hub", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Abyss Engine v2027.Q1 (Endeavor)");
    ImGui::Separator();
    ImGui::TextDisabled("No recent projects found in C:\\AbyssProjects");
    ImGui::End();
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

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();

        // Load Fonts
        io.Fonts->AddFontFromFileTTF("fonts/Noto_Sans/NotoSans-VariableFont_wdth,wght.ttf", 18.0f);
        
        ImGui_ImplGlfw_InitForVulkan(g_window, true);

        if (!g_renderer.init(g_window, mode->width, mode->height)) {
            throw std::runtime_error("Failed to init renderer");
        }

        while (!g_renderer.shouldClose()) {
            g_renderer.pollEvents();

            ImGui_ImplGlfw_NewFrame();
            ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();

            draw_hub_ui();

            ImGui::Render();
            g_renderer.drawFrame();
        }

        g_renderer.waitIdle();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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