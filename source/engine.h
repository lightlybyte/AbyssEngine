#pragma once

#include <string>
#include "vkrender.h"

// Game state
enum class GameState {
    MENU,
    PLAYING,
    PAUSED
};

// Menu sub-screen
enum class MenuScreen {
    MAIN,
    SETTINGS,
    PYTHON_CONSOLE,
    CREDITS
};

namespace Engine {

    void Init(VkRenderer& renderer);
    void Shutdown();
    void Update(VkRenderer& renderer, double dt);
    void RenderGame(VkRenderer& renderer);
    void RenderMenu(VkRenderer& renderer, struct nk_context* ctx);
    GameState GetState();
    void SetState(GameState state);

    // Menu navigation
    MenuScreen GetMenuScreen();
    void SetMenuScreen(MenuScreen screen);

    // Python bridge
    void ExecutePythonCommand(const std::string& command);
    bool IsPythonInitialized();

    // Set the GLFW window used by the menu layer (call after window creation).
    void SetWindow(GLFWwindow* window);

} // namespace Engine