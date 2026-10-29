#include <iostream>
#include <stdexcept>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vkrender.h"
#include "engine.h"
#include "python.h"

static GLFWwindow* g_window = nullptr;
static VkRenderer g_renderer;

// Python bridge callbacks exposed to the embedded interpreter.
extern "C" {

static PyObject* py_echo(PyObject* self, PyObject* args) {
    const char* text = nullptr;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    std::cout << "[PY] " << text << std::endl;
    Py_RETURN_NONE;
}

static PyObject* py_deploy(PyObject* self, PyObject* args) {
    Engine::SetState(GameState::PLAYING);
    std::cout << "[DATS] Deploying to server..." << std::endl;
    Py_RETURN_NONE;
}

static PyObject* py_quit(PyObject* self, PyObject* args) {
    std::cout << "[DATS] Quitting game..." << std::endl;
    Py_RETURN_NONE;
}

static PyObject* py_get_status(PyObject* self, PyObject* args) {
    return PyUnicode_FromString("DATS v1.0 - Tactical Operations Command");
}

} // extern "C"

int main() {
    try {
        std::cout << "[DATS] Booting..." << std::endl;

        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        g_window = glfwCreateWindow(mode->width, mode->height, "DATS", monitor, nullptr);
        if (!g_window) throw std::runtime_error("Failed to create window");

        if (!g_renderer.init(g_window, mode->width, mode->height)) {
            throw std::runtime_error("Failed to init renderer");
        }

        Engine::SetWindow(g_window);
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