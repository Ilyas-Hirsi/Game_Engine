#pragma once
#include "../Scene/Entity.h"
#include <glm/glm.hpp>
namespace engine {

struct CollisionEvent {
  Entity entity_a;
  Entity entity_b;
  glm::vec3 normal;
  glm::vec3 point;
  float penetration;
  float impulse;
  
};

} 
