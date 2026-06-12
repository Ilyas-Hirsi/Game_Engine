#include "Input.h"

namespace engine {
    Input::Input() {
        current_keys_.fill(false);
        previous_keys_.fill(false);
    }
    void Input::BeginFrame() {
        previous_keys_ = current_keys_;
    }
    void Input::SetKeyDown(KeyCode key) {
        current_keys_[static_cast<int>(key)] = true;
    }
    void Input::SetKeyUp(KeyCode key) {
        current_keys_[static_cast<int>(key)] = false;
    }
    bool Input::IsKeyDown(KeyCode key) {
        return current_keys_[static_cast<int>(key)];
    }
    bool Input::WasKeyPressed(KeyCode key) {
        return current_keys_[static_cast<int>(key)] && !previous_keys_[static_cast<int>(key)];
    }
    bool Input::WasKeyReleased(KeyCode key) {
        return !current_keys_[static_cast<int>(key)] && previous_keys_[static_cast<int>(key)];
    }
}