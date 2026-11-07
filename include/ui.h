#pragma once
// ---------------------------------------------------------------------------
// ui.h — single header for the entire DATS menu system.
//
// NOTE: nk_rect is a struct, not a typedef. We forward-declare it with the
// `struct` keyword and always use `struct nk_rect` in the interface so the
// header compiles regardless of how the caller's nuklear.h is wrapped.
// ---------------------------------------------------------------------------

struct nk_context;
struct GLFWwindow;
struct nk_rect;           // <-- struct, not typedef
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

void Render(nk_context* ctx, VkRenderer& renderer, Screen screen,
            int w, int h, GLFWwindow* window);

void Update(float dt);

bool SidebarContains(float x, float y, int w, int h);

void ApplyStyle(nk_context* ctx);

} // namespace Ui