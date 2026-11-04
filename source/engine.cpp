// ---------------------------------------------------------------------------
// engine.cpp — thin orchestration.
//
// CRITICAL: do NOT include <nuklear.h> here. Only ui.cpp includes it.
// Nuklear requires the exact same NK_INCLUDE_* macros every time it is
// included; splitting that across TUs causes the types section to be
// skipped in whichever TU sees the include guard first.
// ---------------------------------------------------------------------------

#include <engine.h>
#include <python.h>
#include <ui.h>
#include <vkrender.h>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cctype>
#include <GLFW/glfw3.h>

// nk_context is forward-declared in engine.h and ui.h. That is enough to
// hold a pointer and pass it into Ui::Render. Nothing here dereferences it.

static GameState g_state       = GameState::MENU;
static MenuScreen g_menuScreen = MenuScreen::MAIN;
static double g_gameTime       = 0.0;
static Settings g_settings;
static GLFWwindow* g_window    = nullptr;
static bool g_quitRequested    = false;

static bool parseBool(const std::string& s) {
    std::string t = s;
    for (auto& c : t) c = (char)std::tolower((unsigned char)c);
    return t == "true" || t == "1" || t == "yes" || t == "on";
}

static Ui::Screen ToUi(MenuScreen s) {
    switch (s) {
        case MenuScreen::MAIN:           return Ui::Screen::Main;
        case MenuScreen::SETTINGS:       return Ui::Screen::Settings;
        case MenuScreen::PYTHON_CONSOLE: return Ui::Screen::Console;
        case MenuScreen::CREDITS:        return Ui::Screen::Credits;
        default:                         return Ui::Screen::Main;
    }
}

namespace Engine {

void Init(VkRenderer& renderer) {
    std::cout << "[DATS] Engine initialized." << std::endl;
    LoadSettings("settings.json");

    if (!PyEngine::Init()) {
        std::cerr << "[DATS] WARNING: Python runtime failed to initialize." << std::endl;
    } else {
        PyEngine::RegisterCallback("engine_echo",
            [](PyObject*, PyObject* args) -> PyObject* {
                const char* text = nullptr;
                if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
                std::cout << "[PY] " << text << std::endl;
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("engine_deploy",
            [](PyObject*, PyObject*) -> PyObject* {
                SetState(GameState::PLAYING);
                std::cout << "[DATS] Deploying to server..." << std::endl;
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("engine_quit",
            [](PyObject*, PyObject*) -> PyObject* {
                RequestQuit();
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("engine_get_status",
            [](PyObject*, PyObject*) -> PyObject* {
                return PyUnicode_FromString("DATS v1.0 - Tactical Operations Command");
            });
        PyEngine::RunFile("scripts/dats_console.py");
    }
    (void)renderer;
}

void Shutdown() {
    PyEngine::Shutdown();
    std::cout << "[DATS] Engine shutdown." << std::endl;
}

GameState GetState() { return g_state; }
void SetState(GameState s) { g_state = s; }

MenuScreen GetMenuScreen() { return g_menuScreen; }
void SetMenuScreen(MenuScreen s) { g_menuScreen = s; }

void RequestQuit() { g_quitRequested = true; }
bool IsQuitRequested() { return g_quitRequested; }

void Update(VkRenderer& /*renderer*/, double dt) {
    if (g_state == GameState::PLAYING) g_gameTime += dt;
    Ui::Update((float)dt);
}

void RenderGame(VkRenderer& renderer) { renderer.drawGameFrame(); }

void ExecutePythonCommand(const std::string& command) {
    if (!PyEngine::IsInitialized()) {
        std::cerr << "[PY] Interpreter not initialized." << std::endl;
        return;
    }
    PyEngine::RunString(command);
}

bool IsPythonInitialized() { return PyEngine::IsInitialized(); }

Settings& GetSettings() { return g_settings; }

void LoadSettings(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    size_t pos = content.find("\"vsync\"");
    if (pos != std::string::npos) {
        size_t colon = content.find(':', pos);
        if (colon != std::string::npos) {
            std::string val = content.substr(colon + 1);
            size_t start = val.find_first_not_of(" \t\n\r");
            if (start != std::string::npos) {
                val = val.substr(start);
                size_t end = val.find_first_of(" ,\n\r}");
                if (end != std::string::npos) val = val.substr(0, end);
                g_settings.vsync = parseBool(val);
            }
        }
    }
}

void SaveSettings(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    file << "{\n  \"vsync\": " << (g_settings.vsync ? "true" : "false") << "\n}\n";
}

void ApplyVSync(VkRenderer& renderer) { renderer.setVSync(g_settings.vsync); }

void SetWindow(GLFWwindow* window) { g_window = window; }

void RenderMenu(VkRenderer& renderer, struct nk_context* ctx) {
    if (!ctx) return;

    int w = 1280, h = 720;
    if (g_window) {
        glfwGetFramebufferSize(g_window, &w, &h);
        if (w <= 0 || h <= 0) { w = 1280; h = 720; }
    }

    Ui::Render(ctx, renderer, ToUi(g_menuScreen), w, h, g_window);

    if (g_quitRequested && g_window) {
        glfwSetWindowShouldClose(g_window, GLFW_TRUE);
        g_quitRequested = false;
    }
}

} // namespace Engine