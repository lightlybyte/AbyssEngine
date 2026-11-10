// ---------------------------------------------------------------------------
// engine.cpp — thin orchestration.
//
// NOTE on includes:
//   Our wrapper is `source/pyengine.h` (renamed from python.h). The old name
//   collided with CPython's `Python.h` on Windows' case-insensitive filesystem
//   — `#include <python.h>` would resolve to CPython's header via -I, so
//   `namespace PyEngine` was never declared.
//
//   `PyEngine` is the wrapper namespace. `pyengine.h` includes <Python.h>
//   and declares it.
// ---------------------------------------------------------------------------

#include "pyengine.h"        // our wrapper, not CPython's Python.h
#include <engine.h>
#include <ui.h>
#include <vkrender.h>
#include <physics.h>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cctype>
#include <GLFW/glfw3.h>

static GameState g_state       = GameState::MENU;
static MenuScreen g_menuScreen = MenuScreen::MAIN;
static double g_gameTime       = 0.0;
static Settings g_settings;
static GLFWwindow* g_window    = nullptr;
static bool g_quitRequested    = false;

// Authentication state
static bool   g_loggedIn    = false;
static std::string g_loggedInUser;
static std::string g_loggedInRole;

// Physics world
static Phys::World g_physicsWorld;

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
        case MenuScreen::LOGIN:          return Ui::Screen::Login;
        case MenuScreen::PROFILE:        return Ui::Screen::Profile;
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
        PyEngine::RegisterCallback("auth_login",
            [](PyObject*, PyObject* args) -> PyObject* {
                const char* username = nullptr;
                const char* password = nullptr;
                if (!PyArg_ParseTuple(args, "ss", &username, &password)) return nullptr;
                // Communicate with the Flask auth server via urllib in the
                // embedded Python interpreter (keeps C++ free of HTTP deps).
                std::string code = R"(
import urllib.request, json
try:
    data = json.dumps({"username": ")" + std::string(username) + R"(", "password": ")" + std::string(password) + R"("}).encode()
    req = urllib.request.Request("http://127.0.0.1/api/login", data=data, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=3) as r:
        result = json.loads(r.read().decode())
        if result.get("ok"):
            print("[AUTH] logged in as " + result.get("username"))
        else:
            print("[AUTH] login failed: " + str(result.get("error")))
except Exception as e:
        print("[AUTH] login error: " + str(e))
)";
                PyEngine::RunString(code);
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("auth_logout",
            [](PyObject*, PyObject*) -> PyObject* {
                std::string code = R"(
import urllib.request
try:
    req = urllib.request.Request("http://127.0.0.1/api/logout", data=b"", method="POST")
    urllib.request.urlopen(req, timeout=3)
    print("[AUTH] logged out")
except Exception as e:
    print("[AUTH] logout error: " + str(e))
)";
                PyEngine::RunString(code);
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("auth_me",
            [](PyObject*, PyObject*) -> PyObject* {
                std::string code = R"(
import urllib.request, json
try:
    req = urllib.request.Request("http://127.0.0.1/api/me")
    with urllib.request.urlopen(req, timeout=3) as r:
        result = json.loads(r.read().decode())
        print("[AUTH] me: " + json.dumps(result))
except Exception as e:
    print("[AUTH] me error: " + str(e))
)";
                PyEngine::RunString(code);
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("engine_set_loggedin",
            [](PyObject*, PyObject* args) -> PyObject* {
                const char* username = nullptr;
                const char* role = nullptr;
                if (!PyArg_ParseTuple(args, "ss", &username, &role)) return nullptr;
                SetLoggedIn(username, role);
                Py_RETURN_NONE;
            });
        PyEngine::RegisterCallback("engine_clear_loggedin",
            [](PyObject*, PyObject*) -> PyObject* {
                ClearLoggedIn();
                Py_RETURN_NONE;
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

// Authentication
bool IsLoggedIn() { return g_loggedIn; }
void SetLoggedIn(const std::string& username, const std::string& role) {
    g_loggedIn    = true;
    g_loggedInUser = username;
    g_loggedInRole = role;
}
void ClearLoggedIn() {
    g_loggedIn    = false;
    g_loggedInUser.clear();
    g_loggedInRole.clear();
}
std::string GetLoggedInUser() { return g_loggedInUser; }
std::string GetLoggedInRole() { return g_loggedInRole; }

// Physics
Phys::World& GetPhysicsWorld() { return g_physicsWorld; }
uint32_t AddPhysicsBody(const Phys::Body& body) { return g_physicsWorld.addBody(body); }
Phys::Body* GetPhysicsBody(uint32_t id) { return g_physicsWorld.getBody(id); }
void RemovePhysicsBody(uint32_t id) { g_physicsWorld.removeBody(id); }

void Update(VkRenderer& /*renderer*/, double dt) {
    if (g_state == GameState::PLAYING) {
        g_gameTime += dt;
        g_physicsWorld.step(static_cast<float>(dt));
    }
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