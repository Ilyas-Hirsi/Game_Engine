#pragma once

#include <cstdint>

namespace engine {
    struct Mesh{
        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int ebo = 0;
        int index_count = 0;
    };
    
    struct MeshHandle {
        std::uint32_t id = 0;
        bool IsValid() const { return id != 0; }
      };
}