#pragma once

#include <variant>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine {
    struct Sphere { float radius; };
    struct Plane  { glm::vec3 normal; float offset; };
    struct Box    { glm::vec3 half_extents; };
    struct Capsule { float radius; float height; };

    struct ChildShape {
      std::variant<Sphere, Plane, Box, Capsule> shape;
      glm::vec3 local_position = glm::vec3(0.0f);
      glm::quat local_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    };

    struct ColliderComponent {
    std::vector<ChildShape> child_shapes;
    bool is_static = false;
};

}  // namespace engine
