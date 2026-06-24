#pragma once
#include <cstdint>
#include <glm/glm.hpp>
namespace engine {
    struct PhysicsComponent {
        std::uint32_t entity_id;
        glm::vec3 position;
        glm::vec3 move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 velocity       = glm::vec3(0.0f);
        glm::vec3 max_velocity   = glm::vec3(100.0f);
        glm::vec3 acceleration   = glm::vec3(0.0f);
        float speed_scalar       = 0.0f;
    };
}  // namespace engine