#include "engine.h"
#include <iostream>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

static GameState g_state = GameState::MENU;
static double g_gameTime = 0.0;

namespace Engine {

void Init(VkRenderer& renderer) {
    std::cout << "[DATS] Engine initialized." << std::endl;
}

void Shutdown() {
    std::cout << "[DATS] Engine shutdown." << std::endl;
}

GameState GetState() { return g_state; }
void SetState(GameState state) { g_state = state; }

void Update(VkRenderer& renderer, double dt) {
    if (g_state == GameState::PLAYING) {
        g_gameTime += dt;
    }
}

void RenderGame(VkRenderer& renderer) {
    // Future: terrain, player, weapons
    renderer.drawGameFrame();
}

void RenderMenu(VkRenderer& renderer, struct nk_context* ctx) {
    nk_glfw3_new_frame();

    // Full-screen military menu
    struct nk_rect full = nk_rect(0, 0, 
        (float)glfwGetVideoMode(glfwGetPrimaryMonitor())->width,
        (float)glfwGetVideoMode(glfwGetPrimaryMonitor())->height);

    nk_style_default(ctx);

    // Dark tactical background
    struct nk_style_window* win = &ctx->style.window;
    win->background = nk_rgba(10, 12, 14, 255);
    win->border_color = nk_rgba(60, 80, 60, 255);

    if (nk_begin(ctx, "DATS", full, NK_WINDOW_NO_SCROLLBAR)) {
        // Header
        nk_layout_row_dynamic(ctx, 40, 1);
        struct nk_style_text* text = &ctx->style.text;
        text->color = nk_rgba(0, 200, 100, 255);
        nk_label(ctx, "DYNAMIC ARMS TRAINING SIMULATOR", NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_label(ctx, "", NK_TEXT_CENTERED);

        // Version / build info
        nk_layout_row_dynamic(ctx, 20, 1);
        text->color = nk_rgba(100, 120, 100, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "", NK_TEXT_CENTERED);

        // Menu options
        nk_layout_row_dynamic(ctx, 45, 1);
        if (nk_button_label(ctx, "DEPLOY")) {
            SetState(GameState::PLAYING);
            std::cout << "[DATS] Deploying..." << std::endl;
        }

        nk_layout_row_dynamic(ctx, 45, 1);
        if (nk_button_label(ctx, "SETTINGS")) {
            std::cout << "[DATS] Settings (stub)" << std::endl;
        }

        nk_layout_row_dynamic(ctx, 45, 1);
        if (nk_button_label(ctx, "QUIT")) {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
        }

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "", NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 20, 1);
        text->color = nk_rgba(80, 80, 80, 255);
        nk_label(ctx, "NO FLAGS. NO BAD GUYS. JUST HUMANS.", NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "THE GEIGER METER RESTS. IN S1.", NK_TEXT_CENTERED);
    }
    nk_end(ctx);
}

} // namespace Engine