#pragma once
// ---------------------------------------------------------------------------
// engine.h — public engine interface.
// ---------------------------------------------------------------------------

#include <string>

struct nk_context;
struct GLFWwindow;
class VkRenderer;

enum class GameState {
    MENU,
    PLAYING,
    PAUSED
};

enum class MenuScreen {
    MAIN,
    SETTINGS,
    PYTHON_CONSOLE,
    CREDITS
};

struct Settings {
    bool vsync = true;
};

namespace Engine {
    void Init(VkRenderer& renderer);
    void Shutdown();

    GameState GetState();
    void SetState(GameState s);

    MenuScreen GetMenuScreen();
    void SetMenuScreen(MenuScreen s);

    void Update(VkRenderer& renderer, double dt);
    void RenderGame(VkRenderer& renderer);
    void RenderMenu(VkRenderer& renderer, struct nk_context* ctx);

    void ExecutePythonCommand(const std::string& command);
    bool IsPythonInitialized();

    Settings& GetSettings();
    void LoadSettings(const std::string& path);
    void SaveSettings(const std::string& path);
    void ApplyVSync(VkRenderer& renderer);

    void SetWindow(GLFWwindow* window);
    void RequestQuit();
    bool IsQuitRequested();
}