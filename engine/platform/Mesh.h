#pragma once

#include <cstdint>
#include <glm/glm.hpp>
namespace engine {
    struct Mesh{
        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ebo = 0;
        int index_count = 0;
        glm::vec3 bounds_min{0.0f}, bounds_max{0.0f};
    };
    
    struct MeshHandle {
        std::uint32_t id = 0;
        bool IsValid() const { return id != 0; }
      };
}
