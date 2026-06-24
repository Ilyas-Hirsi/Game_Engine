#pragma once
#include <cstdint>
#include <cmath>
#include <glm/glm.hpp>

namespace engine {

struct TransformComponent {
  std::uint32_t entity_id;
  glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);

  glm::vec3 facing = glm::vec3(0.0f, 0.0f, -1.0f);

  glm::vec3 move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
  float velocity = 0.0f;
  glm::vec3 max_velocity = glm::vec3(100.0f, 100.0f, 100.0f);
  glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
};

}  // namespace engine
