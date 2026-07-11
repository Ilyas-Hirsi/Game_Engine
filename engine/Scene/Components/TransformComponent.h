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
};

}  // namespace engine
