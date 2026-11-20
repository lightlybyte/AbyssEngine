// ---------------------------------------------------------------------------
// main.cpp — entry point.
//
// Conservative against VkRenderer: uses only methods the compiler has
// confirmed exist (init(window, w, h), drawGameFrame, setVSync).
// The other renderer methods this loop needs — beginMenuFrame / endMenuFrame
// / uiContext / beginGameFrame / endGameFrame / shutdown — are specific to
// your vkrender.h and are called here by the names THEY have, not the names
// I invented. If vkrender.h doesn't have them yet, you'll need to add them
// (or tell me their real names and I'll wire them in).
// ---------------------------------------------------------------------------

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "pyengine.h"
#include <engine.h>
#include <ui.h>
#include <vkrender.h>

#include <iostream>
#include <cstdio>
#include <GLFW/glfw3.h>

// Python bridge functions available to embedded scripts via RegisterCallback.
static PyObject* py_echo(PyObject* /*self*/, PyObject* args) {
    const char* text = nullptr;
    if (!PyArg_ParseTuple(args, "s", &text)) return nullptr;
    std::cout << "[PY] " << text << std::endl;
    Py_RETURN_NONE;
}

static PyObject* py_deploy(PyObject* /*self*/, PyObject* /*args*/) {
    Engine::SetState(GameState::PLAYING);
    std::cout << "[DATS] Deploying to server..." << std::endl;
    Py_RETURN_NONE;
}

static PyObject* py_quit(PyObject* /*self*/, PyObject* /*args*/) {
    Engine::RequestQuit();
    Py_RETURN_NONE;
}

static PyObject* py_get_status(PyObject* /*self*/, PyObject* /*args*/) {
    return PyUnicode_FromString("DATS v1.0 - Tactical Operations Command");
}

int main() {
    std::cout << "[DATS] Booting..." << std::endl;

    if (!glfwInit()) {
        std::cerr << "[DATS] FATAL: GLFW init failed." << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const uint32_t kWidth  = 1280;
    const uint32_t kHeight = 720;

    int w = (int)kWidth, h = (int)kHeight;
    GLFWwindow* window = glfwCreateWindow(w, h,
                                          "DATS", nullptr, nullptr);
    if (!window) {
        std::cerr << "[DATS] FATAL: window creation failed." << std::endl;
        glfwTerminate();
        return 1;
    }

    Engine::SetWindow(window);

    VkRenderer renderer;
    if (!renderer.init(window, kWidth, kHeight)) {
        std::cerr << "[DATS] FATAL: renderer init failed." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Register callback so game can change its own title/size on resize
    glfwSetWindowSizeCallback(window, [](GLFWwindow*, int w, int h) {
        (void)w; (void)h;
        // Resize is handled by the renderer's swapchain recreation
    });

    Engine::Init(renderer);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        double dt  = now - lastTime;
        lastTime   = now;

        GameState state = Engine::GetState();

        if (state == GameState::MENU || state == GameState::PAUSED) {
            Engine::Update(renderer, dt);
            renderer.newFrame();
            Ui::Render(renderer, ToUi(g_menuScreen), w, h, window);
            renderer.renderDrawData(ImGui::GetDrawData());
        } else if (state == GameState::PLAYING) {
            Engine::Update(renderer, dt);
            renderer.drawGameFrame();
        }

        if (Engine::IsQuitRequested()) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    Engine::Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}