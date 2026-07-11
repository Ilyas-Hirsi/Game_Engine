#pragma once

#include <variant>
#include <glm/glm.hpp>

namespace engine {
    struct Sphere { float radius; };
    struct Plane  { glm::vec3 normal; float offset; };
    struct Box    { glm::vec3 half_extents; };

    struct ColliderComponent {
    std::variant<Sphere, Plane, Box> shape;
    bool is_static = false;
};

}  // namespace engine
