#include <iostream>
#include <vector>
#include <string>

// Vulkan
#include <vulkan/vulkan.h>

// GLFW
#include <GLFW/glfw3.h>

// Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

// Wren
#include "wren.h"

// ---------------------------------------------------------
// Engine State (lowercase)
// ---------------------------------------------------------
static GLFWwindow* g_window = nullptr;
static VkInstance g_vk_instance = VK_NULL_HANDLE;

// The path where projects will be stored
static const std::string PROJECTS_DIR = "C:\\AbyssProjects";

struct project_info {
    std::string name;
    std::string path;
    std::string last_opened;
};

// Start empty. We will populate this when we write the project logic.
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

    // Title
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Abyss Engine v2027.Q1 (Endeavor)");
    ImGui::Separator();

    // Projects List
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
                    std::cout << "Opening: " << proj.name << std::endl;
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
    
    // Action Buttons
    if (ImGui::Button("New Project", ImVec2(150, 40))) {
        std::cout << "New Project clicked - logic coming soon" << std::endl;
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Project", ImVec2(150, 40))) {
        std::cout << "Open Project clicked - logic coming soon" << std::endl;
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
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    g_window = glfwCreateWindow(1920, 1080, "Abyss Engine Hub", nullptr, nullptr);
    if (!g_window) {
        std::cerr << "[Abyss Engine] Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 2. Init Vulkan
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Abyss Engine Hub";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    
    uint32_t glfw_ext_count = 0;
    const char** glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    create_info.enabledExtensionCount = glfw_ext_count;
    create_info.ppEnabledExtensionNames = glfw_exts;

    if (vkCreateInstance(&create_info, nullptr, &g_vk_instance) != VK_SUCCESS) {
        std::cerr << "[Abyss Engine] Failed to create Vulkan instance" << std::endl;
        return -1;
    }

    // 3. Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(g_window, true);

    // 4. The Hub Loop
    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_hub_ui();

        ImGui::Render();
        // Rendering to the screen requires the full Vulkan swapchain setup.
    }

    // 5. Cleanup
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyInstance(g_vk_instance, nullptr);
    glfwDestroyWindow(g_window);
    glfwTerminate();

    std::cout << "[Abyss Engine] Hub Shutdown." << std::endl;
    return 0;
}