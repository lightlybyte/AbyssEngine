// Abyss Engine Finite State Machine - Game State Management
// Manages game state transitions: MENU -> PLAYING -> PAUSED -> MENU
// Provides observation pattern for state changes, cleaned up in Shutdown().

#include <iostream>
#include <vector>
#include "fsm.h"

namespace Fsm {

// Static state instance
static State current_state = State::MENU;
static State pending_state = State::MENU;

// Observers - can be game systems, UI, physics, etc.
struct Observer {
    std::string name;
    void (*on_state_change)(State from, State to);
};

static std::vector<Observer> observers;

// Register an observer for state changes
void register_observer(const std::string& name, void (*callback)(State, State)) {
    observers.push_back({name, callback});
}

// Unregister an observer
void unregister_observer(const std::string& name) {
    for (auto it = observers.begin(); it != observers.end(); ++it) {
        if (it->name == name) {
            observers.erase(it);
            break;
        }
    }
}

// Notify all observers of a state change
void notify_observers(State from, State to) {
    for (const auto& obs : observers) {
        if (obs.on_state_change) {
            obs.on_state_change(from, to);
        }
    }
}

// Set the game state
void set_state(State new_state) {
    State old_state = current_state;
    pending_state = new_state;
}

// Update the FSM - process pending state transition
void update() {
    if (pending_state != current_state) {
        current_state = pending_state;
        notify_observers(current_state, pending_state);
    }
}

// Get current state
State get_state() {
    return current_state;
}

// Shutdown - cleanup
void shutdown() {
    observers.clear();
}

} // namespace Fsm
