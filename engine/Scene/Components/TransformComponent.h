#pragma once
#include <cstdint>

namespace engine {
    struct TransformComponent {
        std::uint32_t entity_id;
        float x;
        float y;
        float velocity_x;
        float max_velocity_x = 100.0f;
        float acceleration_x = 100.0f;
        float velocity_y;
        float max_velocity_y = 100.0f;
        float acceleration_y = 100.0f;
        float scale_x;
        float scale_y;
        float rotation;
    };
}