#pragma once
// ---------------------------------------------------------------------------
// ui.h — single header for the entire DATS menu system.
//
// Pure interface: forward declares nk_context and VkRenderer, includes
// nothing heavy. Only ui.cpp pulls in nuklear and draws.
// ---------------------------------------------------------------------------

struct nk_context;
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
    Count
};

// Called once per frame from Engine::RenderMenu. Does framing, sidebar,
// content pane, crossfade — everything.
void Render(nk_context* ctx, VkRenderer& renderer, Screen screen,
            int w, int h, GLFWwindow* window);

// Advances the crossfade. dt in seconds.
void Update(float dt);

// True if (x, y) lies in the sidebar hit region.
bool SidebarContains(float x, float y, int w, int h);

// Applied once after nk_init. Safe to call multiple times.
void ApplyStyle(nk_context* ctx);

} // namespace Ui