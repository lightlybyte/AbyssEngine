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

#include "pyengine.h"
#include <engine.h>
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

    GLFWwindow* window = glfwCreateWindow((int)kWidth, (int)kHeight,
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

    Engine::Init(renderer);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        double dt  = now - lastTime;
        lastTime   = now;

        Engine::Update(renderer, dt);

        // ------------------------------------------------------------------
        // Frame submission.
        //
        // The VkRenderer API around here is project-specific. What this loop
        // MUST do, in order:
        //
        //   MENU state:
        //     1. begin a UI frame (vkCmdBeginRendering / acquire swapchain image)
        //     2. Engine::RenderMenu(renderer, nk_ctx)  -- draws nuklear widgets
        //     3. submit the UI vertex buffer + present
        //
        //   PLAYING state:
        //     1. begin a game frame
        //     2. Engine::RenderGame(renderer)  -- draws scene
        //     3. present
        //
        // Your VkRenderer already knows how to draw the game frame
        // (`drawGameFrame`), so the PLAYING branch below works today. The MENU
        // branch needs whatever method names vkrender.h uses to (a) hand you
        // the nuklear context and (b) submit the UI. Common names in similar
        // renderers are: beginFrame/endFrame, submitUI, uiContext, nkContext.
        //
        // Replace the calls marked TODO below with your real ones.
        // ------------------------------------------------------------------

        if (Engine::GetState() == GameState::MENU) {
            renderer.beginUIFrame();
            Engine::RenderMenu(renderer, renderer.ctx());
            renderer.endUIFrame();
        } else if (Engine::GetState() == GameState::PLAYING) {
            Engine::RenderGame(renderer);
        }
    }

    Engine::Shutdown();

    // TODO: renderer.shutdown() or renderer.destroy() — whatever your
    //       vkrender.h exposes for teardown. If it's a destructor, this line
    //       can be removed entirely.

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}