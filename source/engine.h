#pragma once
// ---------------------------------------------------------------------------
// engine.h — public engine interface.
// ---------------------------------------------------------------------------

#include <string>
#include <cstdint>

struct nk_context;
struct GLFWwindow;
class VkRenderer;
namespace Phys { class World; struct Body; }

enum class GameState {
    MENU,
    PLAYING,
    PAUSED
};

enum class MenuScreen {
    MAIN,
    SETTINGS,
    PYTHON_CONSOLE,
    CREDITS,
    LOGIN,
    PROFILE
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

    // Authentication
    bool   IsLoggedIn();
    void   SetLoggedIn(const std::string& username, const std::string& role);
    void   ClearLoggedIn();
    std::string GetLoggedInUser();
    std::string GetLoggedInRole();

    // Physics
    Phys::World& GetPhysicsWorld();
    uint32_t AddPhysicsBody(const Phys::Body& body);
    Phys::Body* GetPhysicsBody(uint32_t id);
    void RemovePhysicsBody(uint32_t id);
};