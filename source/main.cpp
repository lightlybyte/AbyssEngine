#include <iostream>
#include <vector>
#include <string>

// GLFW
#include "GLFW/glfw3.h"

// Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

// Abyss Renderer
#include "vkrender.h"

// ---------------------------------------------------------
// Engine State (lowercase)
// ---------------------------------------------------------
static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

static const std::string PROJECTS_DIR = "C:\\AbyssProjects";

// Language state
static int g_current_language = 0;
static const char* g_languages[] = { "English", "Chinese (Simplified)", "Japanese", "Korean" };

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
        
        // Right-aligned Language Selector
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SameLine(windowWidth - 200);
        ImGui::SetNextItemWidth(150);
        ImGui::Combo("##Language", &g_current_language, g_languages, IM_ARRAYSIZE(g_languages));
        
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
        // ... (Project list logic)
    }

    ImGui::Spacing();
    ImGui::Separator();
    
    if (ImGui::Button("New Project", ImVec2(150, 40))) {}
    ImGui::SameLine();
    if (ImGui::Button("Open Project", ImVec2(150, 40))) {}
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
    if (!glfwInit()) return -1;

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    
    g_window = glfwCreateWindow(mode->width, mode->height, "Abyss Engine Hub", nullptr, nullptr);
    if (!g_window) return -1;

    // 2. Init ImGui Core
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // --- LOAD FONTS ---
    // Primary font (Noto Sans Latin)
    ImFont* font_main = io.Fonts->AddFontFromFileTTF("fonts/Noto_Sans/NotoSans-VariableFont_wdth,wght.ttf", 18.0f);
    
    // Fallback chain for CJK and Emoji
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0xFFFD, 0xFFFD, // Invalid
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };
    
    ImFontConfig config;
    config.MergeMode = true; // Merge into the primary font
    
    io.Fonts->AddFontFromFileTTF("fonts/Noto_Sans_SC/NotoSansSC-VariableFont_wght.ttf", 18.0f, &config, ranges);
    io.Fonts->AddFontFromFileTTF("fonts/Noto_Sans_JP/NotoSansJP-VariableFont_wght.ttf", 18.0f, &config, ranges);
    io.Fonts->AddFontFromFileTTF("fonts/Noto_Sans_KR/NotoSansKR-VariableFont_wght.ttf", 18.0f, &config, ranges);
    io.Fonts->AddFontFromFileTTF("fonts/Noto_Color_Emoji/NotoColorEmoji-Regular.ttf", 18.0f, &config, ranges);
    
    // Set as default
    io.FontDefault = font_main;
    // -----------------

    // 3. Init GLFW Backend
    ImGui_ImplGlfw_InitForVulkan(g_window, true);

    // 4. Init Renderer
    if (!g_renderer.init(g_window)) {
        std::cerr << "[Abyss Engine] Failed to init renderer" << std::endl;
        return -1;
    }

    // 5. The Hub Loop
    while (!glfwWindowShouldClose(g_window)) {
        glfwPollEvents();

        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
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

    return 0;
}