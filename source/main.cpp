#include <iostream>
#include <vector>
#include <string>

// GLFW
#include "GLFW/glfw3.h"

// Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

// Wren
#include "wren.h"

// Abyss Renderer
#include "vkrender.h"

// ---------------------------------------------------------
// Engine State (lowercase)
// ---------------------------------------------------------
static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

static const std::string PROJECTS_DIR = "C:\\AbyssProjects";

struct project_info {
    std::string name;
    std::string path;
    std::string last_opened;
};

static std::vector<project_info> g_recent_projects = {};

// ---------------------------------------------------------
// Hub UI (PascalCase text)
// ---------------------------------------------------------
static void draw_hub_ui() {
    // 1. The Top Bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project")) {}
            if (ImGui::MenuItem("Open Project")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(g_window, true);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) { ImGui::EndMenu(); }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Abyss Engine")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // 2. The Main Hub Window
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Always);
    
    ImGui::Begin("Abyss Engine Hub", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Abyss Engine v2027.Q1 (Endeavor)");
    ImGui::Separator();

    ImGui::Text("Projects");
    
    if (g_recent_projects.empty()) {
        ImGui::TextDisabled("No recent projects found in %s", PROJECTS_DIR.c_str());
    } else {
        if (ImGui::BeginTable("ProjectsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Project Name");
            ImGui::TableSetupColumn("Path");
            ImGui::TableSetupColumn("Last Opened");
            ImGui::TableHeadersRow();

            for (const auto& proj : g_recent_projects) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(proj.name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    // Handle project open here
                }
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", proj.path.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", proj.last_opened.c_str());
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    
    if (ImGui::Button("New Project", ImVec2(150, 40))) {
        // Logic coming soon
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Project", ImVec2(150, 40))) {
        // Logic coming soon
    }
    ImGui::SameLine();
    if (ImGui::Button("Quit", ImVec2(150, 40))) {
        glfwSetWindowShouldClose(g_window, true);
    }

    ImGui::End();
}

// ---------------------------------------------------------
// Main Entry Point
// ---------------------------------------------------------
int main() {
    std::cout << "[Abyss Engine] Booting Hub..." << std::endl;

    // 1. Init GLFW
    if (!glfwInit()) {
        std::cerr << "[Abyss Engine] Failed to init GLFW" << std::endl;
        return -1;
    }

    // Fullscreen Windowed Setup
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Borderless
    
    g_window = glfwCreateWindow(mode->width, mode->height, "Abyss Engine Hub", nullptr, nullptr);
    
    if (!g_window) {
        std::cerr << "[Abyss Engine] Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 2. Init ImGui Core FIRST (Fixes the assertion)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // 3. Init GLFW Backend
    ImGui_ImplGlfw_InitForVulkan(g_window, true);

    // 4. Init Renderer (Vulkan + ImGui Vulkan Backend)
    if (!g_renderer.init(g_window)) {
        std::cerr << "[Abyss Engine] Failed to init renderer" << std::endl;
        return -1;
    }

    // 5. The Hub Loop
    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_hub_ui();

        ImGui::Render();
        
        g_renderer.draw_frame();
    }

    // 6. Cleanup
    g_renderer.shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_window);
    glfwTerminate();

    std::cout << "[Abyss Engine] Hub Shutdown." << std::endl;
    return 0;
}