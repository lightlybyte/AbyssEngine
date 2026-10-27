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
    renderer.drawGameFrame();
}

void RenderMenu(VkRenderer& renderer, struct nk_context* ctx) {
    // NOTE: nk_glfw3_new_frame() is handled in main.cpp/loop. 
    // Do not call it here to avoid double-framing.

    // 1. Tactical Dark Theme
    struct nk_color background = nk_rgba(10, 12, 14, 255); // Very dark grey/black
    struct nk_color border = nk_rgba(60, 80, 60, 255);     // Military green border
    struct nk_color text = nk_rgba(200, 220, 200, 255);    // Off-white text
    struct nk_color button_normal = nk_rgba(30, 40, 30, 255);
    struct nk_color button_hover = nk_rgba(50, 70, 50, 255);
    struct nk_color button_active = nk_rgba(80, 120, 80, 255);

    nk_style_default(ctx);
    
    // Window styling
    ctx->style.window.background = background;
    ctx->style.window.border_color = border;
    ctx->style.window.border = 2.0f;
    ctx->style.window.fixed_background = nk_style_item_color(background);

    // Text styling
    ctx->style.text.color = text;

    // Button styling
    ctx->style.button.normal = nk_style_item_color(button_normal);
    ctx->style.button.hover = nk_style_item_color(button_hover);
    ctx->style.button.active = nk_style_item_color(button_active);
    ctx->style.button.border_color = border;
    ctx->style.button.text_normal = text;
    ctx->style.button.text_hover = nk_rgba(255, 255, 255, 255);
    ctx->style.button.text_active = nk_rgba(255, 255, 255, 255);

    // 2. Full Screen Layout
    // Get the actual screen size from the renderer/window
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);

    struct nk_rect full_screen = nk_rect(0, 0, (float)width, (float)height);

    // We use NO_SCROLLBAR and NO_TITLE to make it feel like a game menu
    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND;

    if (nk_begin(ctx, "DATS_MAIN", full_screen, flags)) {
        
        // Push everything down and to the left
        nk_layout_row_dynamic(ctx, (float)height * 0.2f, 1); // Top margin

        // Header
        nk_layout_row_dynamic(ctx, 60, 1);
        ctx->style.text.color = nk_rgba(0, 255, 100, 255); // Bright green for title
        nk_label(ctx, "DYNAMIC ARMS TRAINING SIMULATOR", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 20, 1);
        ctx->style.text.color = nk_rgba(100, 120, 100, 255);
        nk_label(ctx, "// TACTICAL OPERATIONS COMMAND", NK_TEXT_LEFT);

        nk_layout_row_dynamic(ctx, 50, 1); // Spacer

        // Buttons (Left-centered block)
        struct nk_rect button_rect = nk_rect(50, 300, 300, 50);
        
        // We use a fixed position layout for the buttons to keep them left-aligned
        nk_layout_space_begin(ctx, NK_STATIC, 300, 4);
        
        nk_layout_space_push(ctx, nk_rect(50, 0, 300, 50));
        if (nk_button_label(ctx, "DEPLOY")) {
            SetState(GameState::PLAYING);
            std::cout << "[DATS] Deploying to server..." << std::endl;
        }

        nk_layout_space_push(ctx, nk_rect(50, 60, 300, 50));
        if (nk_button_label(ctx, "SETTINGS")) {
            std::cout << "[DATS] Settings menu (stub)" << std::endl;
        }

        nk_layout_space_push(ctx, nk_rect(50, 120, 300, 50));
        if (nk_button_label(ctx, "QUIT")) {
            glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
        }

        nk_layout_space_end(ctx);

        // Footer (Bottom of screen)
        nk_layout_row_dynamic(ctx, (float)height * 0.3f, 1); // Push to bottom
        ctx->style.text.color = nk_rgba(80, 80, 80, 255);
        nk_label(ctx, "BUILD: 2027.Q1 // ABYSS ENGINE", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "NO FLAGS. NO BAD GUYS. JUST HUMANS.", NK_TEXT_LEFT);
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "THE GEIGER METER RESTS. IN S1.", NK_TEXT_LEFT);
    }
    nk_end(ctx);
}

} // namespace Engine