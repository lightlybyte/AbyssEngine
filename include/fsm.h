// Abyss Engine Finite State Machine - Game State Management
// Header for the state machine used to manage game states
// (MENU, PLAYING, PAUSED, MENU)

#pragma once

namespace Fsm {

enum class State {
    MENU,
    PLAYING,
    PAUSED,
    EXITED
};

// Register an observer for state changes
void register_observer(const std::string& name, void (*callback)(State, State));

// Unregister an observer
void unregister_observer(const std::string& name);

// Set the game state
void set_state(State new_state);

// Update the FSM - process pending state transitions
void update();

// Get current state
State get_state();

// Shutdown
void shutdown();

} // namespace Fsm
