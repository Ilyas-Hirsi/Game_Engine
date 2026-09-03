#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
namespace engine {
    struct RigidBodyComponent {
        glm::vec3 previous_position = glm::vec3(0.0f);
        glm::quat previous_rotation_quat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 linear_velocity       = glm::vec3(0.0f);
        glm::vec3 acceleration   = glm::vec3(0.0f);
        glm::vec3 angular_velocity = glm::vec3(0.0f);
        glm::vec3 angular_acceleration = glm::vec3(0.0f);
        glm::vec3 inverse_inertia = glm::vec3(0.0f);
        float gravity_scale = 1.0f;
        float restitution = 0.2f;
        float inverse_mass = 1.0f;
    };
    struct MovementComponent {
        glm::vec3 move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 facing = glm::vec3(0.0f, 0.0f, -1.0f);
        float speed              = 0.0f;
        float yaw                = 0.0f;
        float pitch              = 0.0f;
    };
}  // namespace engine