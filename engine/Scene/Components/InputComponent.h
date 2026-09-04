#pragma once
#include <cstdint>
#include "../../platform/Input/KeyCode.h"
namespace engine {
    struct InputComponent {
        KeyCode up = KeyCode::W;
        KeyCode down = KeyCode::S;
        KeyCode left = KeyCode::A;
        KeyCode right = KeyCode::D;
        KeyCode rotate_left = KeyCode::Left;
        KeyCode rotate_right = KeyCode::Right;
        KeyCode rotate_up = KeyCode::Up;
        KeyCode rotate_down = KeyCode::Down;
        KeyCode camera_up = KeyCode::Q;
        KeyCode camera_down = KeyCode::E;
    };
}