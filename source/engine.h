#pragma once

#include "vkrender.h"

// Game state
enum class GameState {
    MENU,
    PLAYING,
    PAUSED
};

namespace Engine {
    void Init(VkRenderer& renderer);
    void Shutdown();
    void Update(VkRenderer& renderer, double dt);
    void RenderGame(VkRenderer& renderer);
    void RenderMenu(VkRenderer& renderer, struct nk_context* ctx);
    GameState GetState();
    void SetState(GameState state);
}