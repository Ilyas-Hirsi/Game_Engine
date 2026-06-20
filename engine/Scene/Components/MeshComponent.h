#pragma once
#include <cstdint>
#include "../../platform/Mesh.h"
#include "../../platform/Texture.h"

namespace engine {

struct MeshComponent {
    std::uint32_t entity_id = 0;
    MeshHandle mesh;
    TextureHandle texture;
};

}  // namespace engine
