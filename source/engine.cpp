#include "engine.h"
#include "python.h"
#include <iostream>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

static GameState g_state = GameState::MENU;
static MenuScreen g_menuScreen = MenuScreen::MAIN;
static double g_gameTime = 0.0;

static GLFWwindow* g_window = nullptr;

namespace Engine {

void Init(VkRenderer& renderer) {
    std::cout << "[DATS] Engine initialized." << std::endl;

    if (!PyEngine::Init()) {
        std::cerr << "[DATS] WARNING: Python runtime failed to initialize."
                  << std::endl;
    } else {
        PyEngine::RegisterCallback("engine_echo",
                                   [](PyObject* self, PyObject* args) -> PyObject* {
                                       const char* text = nullptr;
                                       if (!PyArg_ParseTuple(args, "s", &text))
                                           return nullptr;
                                       std::cout << "[PY] " << text << std::endl;
                                       Py_RETURN_NONE;
                                   });

        PyEngine::RegisterCallback("engine_deploy",
                                   [](PyObject* self, PyObject* args) -> PyObject* {
                                       SetState(GameState::PLAYING);
                                       std::cout << "[DATS] Deploying to server..." << std::endl;
                                       Py_RETURN_NONE;
                                   });

        PyEngine::RegisterCallback("engine_quit",
                                   [](PyObject* self, PyObject* args) -> PyObject* {
                                       std::cout << "[DATS] Quitting game..." << std::endl;
                                       Py_RETURN_NONE;
                                   });

        PyEngine::RegisterCallback("engine_get_status",
                                   [](PyObject* self, PyObject* args) -> PyObject* {
                                       return PyUnicode_FromString("DATS v1.0 - Tactical Operations Command");
                                   });

        PyEngine::RunFile("scripts/dats_console.py");
    }
}

void Shutdown() {
    PyEngine::Shutdown();
    std::cout << "[DATS] Engine shutdown." << std::endl;
}

GameState GetState() { return g_state; }
void SetState(GameState state) { g_state = state; }

MenuScreen GetMenuScreen() { return g_menuScreen; }
void SetMenuScreen(MenuScreen screen) { g_menuScreen = screen; }

void Update(VkRenderer& renderer, double dt) {
    if (g_state == GameState::PLAYING) {
        g_gameTime += dt;
    }
}

void RenderGame(VkRenderer& renderer) {
    renderer.drawGameFrame();
}

void ExecutePythonCommand(const std::string& command) {
    if (!PyEngine::IsInitialized()) {
        std::cerr << "[PY] Interpreter not initialized." << std::endl;
        return;
    }
    PyEngine::RunString(command);
}

bool IsPythonInitialized() {
    return PyEngine::IsInitialized();
}

} // namespace Engine

// ---------------------------------------------------------------------------
// Forward declarations for static menu sub-screen renderers
// ---------------------------------------------------------------------------

static void renderMainMenu(struct nk_context* ctx, int width, int height);
static void renderSettingsMenu(struct nk_context* ctx, int width, int height);
static void renderPythonConsole(struct nk_context* ctx, int width, int height);
static void renderCreditsMenu(struct nk_context* ctx, int width, int height);

void Engine::RenderMenu(VkRenderer& renderer, struct nk_context* ctx) {
    if (!g_window) {
        std::cerr << "[DATS] ERROR: GLFW window not set for menu rendering."
                  << std::endl;
        return;
    }

    int width = 0, height = 0;
    glfwGetFramebufferSize(g_window, &width, &height);
    if (width <= 0 || height <= 0) {
        glfwGetWindowSize(g_window, &width, &height);
    }
    if (width <= 0) width = 1280;
    if (height <= 0) height = 720;

    switch (Engine::GetMenuScreen()) {
        case MenuScreen::MAIN:
            renderMainMenu(ctx, width, height);
            break;
        case MenuScreen::SETTINGS:
            renderSettingsMenu(ctx, width, height);
            break;
        case MenuScreen::PYTHON_CONSOLE:
            renderPythonConsole(ctx, width, height);
            break;
        case MenuScreen::CREDITS:
            renderCreditsMenu(ctx, width, height);
            break;
    }
}

static void renderMainMenu(struct nk_context* ctx, int width, int height) {
    struct nk_color background = nk_rgba(10, 12, 14, 255);
    struct nk_color border = nk_rgba(60, 80, 60, 255);
    struct nk_color text = nk_rgba(200, 220, 200, 255);
    struct nk_color button_normal = nk_rgba(30, 40, 30, 255);
    struct nk_color button_hover = nk_rgba(50, 70, 50, 255);
    struct nk_color button_active = nk_rgba(80, 120, 80, 255);

    nk_style_default(ctx);

    ctx->style.window.background = background;
    ctx->style.window.border_color = border;
    ctx->style.window.border = 2.0f;
    ctx->style.window.fixed_background = nk_style_item_color(background);

    ctx->style.text.color = text;

    ctx->style.button.normal = nk_style_item_color(button_normal);
    ctx->style.button.hover = nk_style_item_color(button_hover);
    ctx->style.button.active = nk_style_item_color(button_active);
    ctx->style.button.border_color = border;
    ctx->style.button.text_normal = text;
    ctx->style.button.text_hover = nk_rgba(255, 255, 255, 255);
    ctx->style.button.text_active = nk_rgba(255, 255, 255, 255);

    struct nk_rect full_screen = nk_rect(0, 0, (float)width, (float)height);
    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND;

    if (nk_begin(ctx, "DATS_MAIN", full_screen, flags)) {
        nk_layout_row_dynamic(ctx, (float)height * 0.15f, 1);

        nk_layout_row_dynamic(ctx, 70, 1);
        ctx->style.text.color = nk_rgba(0, 255, 100, 255);
        nk_label(ctx, "DYNAMIC ARMS TRAINING SIMULATOR", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 24, 1);
        ctx->style.text.color = nk_rgba(100, 120, 100, 255);
        nk_label(ctx, "// TACTICAL OPERATIONS COMMAND", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, (float)height * 0.10f, 1);

        nk_layout_space_begin(ctx, NK_STATIC, 400, 4);

        nk_layout_space_push(ctx, nk_rect(50, 0, 300, 55));
        if (nk_button_label(ctx, "DEPLOY")) {
            Engine::SetState(GameState::PLAYING);
            std::cout << "[DATS] Deploying to server..." << std::endl;
        }

        nk_layout_space_push(ctx, nk_rect(50, 65, 300, 55));
        if (nk_button_label(ctx, "SETTINGS")) {
            Engine::SetMenuScreen(MenuScreen::SETTINGS);
        }

        nk_layout_space_push(ctx, nk_rect(50, 130, 300, 55));
        if (nk_button_label(ctx, "PYTHON CONSOLE")) {
            Engine::SetMenuScreen(MenuScreen::PYTHON_CONSOLE);
        }

        nk_layout_space_push(ctx, nk_rect(50, 195, 300, 55));
        if (nk_button_label(ctx, "CREDITS")) {
            Engine::SetMenuScreen(MenuScreen::CREDITS);
        }

        nk_layout_space_push(ctx, nk_rect(50, 260, 300, 55));
        if (nk_button_label(ctx, "QUIT")) {
            glfwSetWindowShouldClose(g_window, true);
        }

        nk_layout_space_end(ctx);

        nk_layout_row_dynamic(ctx, (float)height * 0.25f, 1);
        ctx->style.text.color = nk_rgba(80, 80, 80, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_LEFT);
        nk_label(ctx, "PYTHON EMBEDDED: ENABLED", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}

static void renderSettingsMenu(struct nk_context* ctx, int width, int height) {
    struct nk_color background = nk_rgba(14, 16, 18, 255);
    struct nk_color border = nk_rgba(80, 100, 80, 255);
    struct nk_color text = nk_rgba(200, 220, 200, 255);

    nk_style_default(ctx);
    ctx->style.window.background = background;
    ctx->style.window.border_color = border;
    ctx->style.window.border = 2.0f;
    ctx->style.text.color = text;

    struct nk_rect full_screen = nk_rect(0, 0, (float)width, (float)height);
    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND;

    if (nk_begin(ctx, "DATS_SETTINGS", full_screen, flags)) {
        nk_layout_row_dynamic(ctx, (float)height * 0.15f, 1);

        nk_layout_row_dynamic(ctx, 60, 1);
        ctx->style.text.color = nk_rgba(0, 255, 100, 255);
        nk_label(ctx, "SETTINGS", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 30, 1);
        ctx->style.text.color = nk_rgba(150, 170, 150, 255);
        nk_label(ctx, "Vulkan renderer: ACTIVE", NK_TEXT_LEFT);
        nk_label(ctx, "Nuklear UI: ACTIVE", NK_TEXT_LEFT);
        nk_label(ctx, "Python runtime: LOADED", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 60, 1);
        if (nk_button_label(ctx, "BACK")) {
            Engine::SetMenuScreen(MenuScreen::MAIN);
        }

        nk_layout_row_dynamic(ctx, (float)height * 0.3f, 1);
        ctx->style.text.color = nk_rgba(80, 80, 80, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}


static void renderPythonConsole(struct nk_context* ctx, int width, int height) {
    struct nk_color background = nk_rgba(8, 10, 12, 255);
    struct nk_color border = nk_rgba(60, 80, 100, 255);
    struct nk_color text = nk_rgba(0, 255, 150, 255);

    nk_style_default(ctx);
    ctx->style.window.background = background;
    ctx->style.window.border_color = border;
    ctx->style.window.border = 2.0f;
    ctx->style.text.color = text;

    struct nk_rect full_screen = nk_rect(0, 0, (float)width, (float)height);
    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND;

    if (nk_begin(ctx, "DATS_PYTHON", full_screen, flags)) {
        nk_layout_row_dynamic(ctx, (float)height * 0.12f, 1);

        nk_layout_row_dynamic(ctx, 50, 1);
        ctx->style.text.color = nk_rgba(0, 255, 100, 255);
        nk_label(ctx, "PYTHON CONSOLE", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 24, 1);
        ctx->style.text.color = nk_rgba(100, 150, 100, 255);
        nk_label(ctx, "Embedded Python 3.12 // Type commands below", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 24, 1);
        ctx->style.text.color = Engine::IsPythonInitialized()
                                    ? nk_rgba(0, 255, 100, 255)
                                    : nk_rgba(255, 80, 80, 255);
        nk_label(ctx, Engine::IsPythonInitialized()
                          ? "STATUS: INTERPRETER ACTIVE"
                          : "STATUS: INTERPRETER FAILED",
                 NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, (float)height * 0.28f, 1);

        ctx->style.text.color = nk_rgba(200, 220, 200, 255);
        nk_label(ctx, "> import sys\n> print('DATS Python bridge online')\nDATS Python bridge online", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 60, 1);
        static char pyInput[256] = {0};
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, pyInput,
                                       sizeof(pyInput), nk_filter_default);

        nk_layout_row_dynamic(ctx, 50, 1);
        if (nk_button_label(ctx, "EXECUTE")) {
            if (Engine::IsPythonInitialized()) {
                Engine::ExecutePythonCommand(pyInput);
            }
            pyInput[0] = '\0';
        }

        if (nk_button_label(ctx, "BACK")) {
            Engine::SetMenuScreen(MenuScreen::MAIN);
        }

        nk_layout_row_dynamic(ctx, (float)height * 0.2f, 1);
        ctx->style.text.color = nk_rgba(80, 80, 80, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}


static void renderCreditsMenu(struct nk_context* ctx, int width, int height) {
    struct nk_color background = nk_rgba(12, 14, 16, 255);
    struct nk_color border = nk_rgba(100, 80, 60, 255);
    struct nk_color text = nk_rgba(220, 200, 180, 255);

    nk_style_default(ctx);
    ctx->style.window.background = background;
    ctx->style.window.border_color = border;
    ctx->style.window.border = 2.0f;
    ctx->style.text.color = text;

    struct nk_rect full_screen = nk_rect(0, 0, (float)width, (float)height);
    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND;

    if (nk_begin(ctx, "DATS_CREDITS", full_screen, flags)) {
        nk_layout_row_dynamic(ctx, (float)height * 0.15f, 1);

        nk_layout_row_dynamic(ctx, 60, 1);
        ctx->style.text.color = nk_rgba(255, 160, 80, 255);
        nk_label(ctx, "CREDITS", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 28, 1);
        ctx->style.text.color = nk_rgba(200, 200, 200, 255);
        nk_label(ctx, "DATS - Dynamic Arms Training Simulator", NK_TEXT_LEFT);
        nk_label(ctx, "Built with Abyss Engine vQ1.2026.1", NK_TEXT_LEFT);
        nk_label(ctx, "Rendering: Vulkan 1.3 + Nuklear UI", NK_TEXT_LEFT);
        nk_label(ctx, "Scripting: Embedded Python 3.12", NK_TEXT_LEFT);
        nk_label(ctx, "Made with a ~$0.79 budget (unspent)", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 60, 1);
        if (nk_button_label(ctx, "BACK")) {
            Engine::SetMenuScreen(MenuScreen::MAIN);
        }

        nk_layout_row_dynamic(ctx, (float)height * 0.25f, 1);
        ctx->style.text.color = nk_rgba(100, 80, 60, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}

void Engine::SetWindow(GLFWwindow* window) {
    g_window = window;
}