#pragma once
#include <array>
#include "KeyCode.h"

namespace engine {
    class Input {
        public:
        Input();
        void BeginFrame();
        void SetKeyDown(KeyCode key);
        void SetKeyUp(KeyCode key);
        bool IsKeyDown(KeyCode key);
        bool WasKeyPressed(KeyCode key);
        bool WasKeyReleased(KeyCode key);

        void SetMouseButtonDown(MouseButton button);
        void SetMouseButtonUp(MouseButton button);
        bool IsMouseButtonDown(MouseButton button);
        bool WasMouseButtonPressed(MouseButton button);
        bool WasMouseButtonReleased(MouseButton button);

        // Window coordinates, origin top left.
        void SetMousePosition(int x, int y);
        void AddMouseDelta(int dx, int dy);
        void AddMouseWheel(float delta);

        int MouseX() const { return mouse_x_; }
        int MouseY() const { return mouse_y_; }
        int MouseDeltaX() const { return mouse_dx_; }
        int MouseDeltaY() const { return mouse_dy_; }
        float MouseWheel() const { return mouse_wheel_; }
        private:
        std::array<bool, static_cast<int>(KeyCode::Count)> current_keys_;
        std::array<bool, static_cast<int>(KeyCode::Count)> previous_keys_;
        std::array<bool, static_cast<int>(MouseButton::Count)> current_buttons_;
        std::array<bool, static_cast<int>(MouseButton::Count)> previous_buttons_;
        int mouse_x_ = 0;
        int mouse_y_ = 0;
        int mouse_dx_ = 0;
        int mouse_dy_ = 0;
        float mouse_wheel_ = 0.0f;
    };
}
