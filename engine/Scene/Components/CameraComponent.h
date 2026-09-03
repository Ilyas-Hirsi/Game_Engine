#pragma once
#include <cstdint>
namespace engine{
    struct CameraComponent {
        float fov = 45.0f;
        float aspect_ratio = 16.0f / 9.0f;
        float near_plane = 0.1f;
        float far_plane = 100.0f;
    };
}  // namespace engine