#pragma once
#include <cstdint>
#include "../../platform/Input/KeyCode.h"
namespace engine {
    struct InputComponent {
        std::uint32_t entity_id;
        KeyCode up = KeyCode::W;
        KeyCode down = KeyCode::S;
        KeyCode left = KeyCode::A;
        KeyCode right = KeyCode::D;
    };
}