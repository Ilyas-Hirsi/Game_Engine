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
        private:
        std::array<bool, static_cast<int>(KeyCode::Count)> current_keys_;
        std::array<bool, static_cast<int>(KeyCode::Count)> previous_keys_;
        ;
    };
}