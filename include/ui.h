#pragma once
// ---------------------------------------------------------------------------
// ui.h — single header for the entire DATS menu system (Dear ImGui version).
// ---------------------------------------------------------------------------

struct GLFWwindow;
class VkRenderer;

namespace Ui {

enum class Screen {
    Main = 0,
    Operations,
    Settings,
    Console,
    Archive,
    Credits,
    Login,
    Profile,
    Count
};

void Render(VkRenderer& renderer, Screen screen, int w, int h, GLFWwindow* window);

void Update(float dt);

bool SidebarContains(float x, float y, int w, int h);

} // namespace Ui