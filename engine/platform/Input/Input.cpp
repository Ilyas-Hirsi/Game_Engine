#include "Input.h"

namespace engine {
    Input::Input() {
        current_keys_.fill(false);
        previous_keys_.fill(false);
        current_buttons_.fill(false);
        previous_buttons_.fill(false);
    }
    void Input::BeginFrame() {
        previous_keys_ = current_keys_;
        previous_buttons_ = current_buttons_;
        // Deltas are per-frame movement, not state, so they do not carry over.
        mouse_dx_ = 0;
        mouse_dy_ = 0;
        mouse_wheel_ = 0.0f;
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

    void Input::SetMouseButtonDown(MouseButton button) {
        current_buttons_[static_cast<int>(button)] = true;
    }
    void Input::SetMouseButtonUp(MouseButton button) {
        current_buttons_[static_cast<int>(button)] = false;
    }
    bool Input::IsMouseButtonDown(MouseButton button) {
        return current_buttons_[static_cast<int>(button)];
    }
    bool Input::WasMouseButtonPressed(MouseButton button) {
        return current_buttons_[static_cast<int>(button)] && !previous_buttons_[static_cast<int>(button)];
    }
    bool Input::WasMouseButtonReleased(MouseButton button) {
        return !current_buttons_[static_cast<int>(button)] && previous_buttons_[static_cast<int>(button)];
    }

    void Input::SetMousePosition(int x, int y) {
        mouse_x_ = x;
        mouse_y_ = y;
    }
    void Input::AddMouseDelta(int dx, int dy) {
        mouse_dx_ += dx;
        mouse_dy_ += dy;
    }
    void Input::AddMouseWheel(float delta) {
        mouse_wheel_ += delta;
    }
}
