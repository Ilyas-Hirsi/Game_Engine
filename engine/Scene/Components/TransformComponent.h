#pragma once
#include <cstdint>

namespace engine {
    struct TransformComponent {
        std::uint32_t entity_id;
        float x;
        float y;
        float z;
        float rotation_x;
        float rotation_y;
        float rotation_z;
        float velocity_x;
        float velocity_y;
        float velocity_z;
        float max_velocity_x = 100.0f;
        float max_velocity_y = 100.0f;
        float max_velocity_z = 100.0f;
        float acceleration_x = 100.0f;
        float acceleration_y = 100.0f;
        float acceleration_z = 100.0f;
        float scale_x = 1.0f;
        float scale_y = 1.0f;
        float scale_z = 1.0f;
    };
}