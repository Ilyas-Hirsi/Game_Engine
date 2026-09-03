#pragma once
#include <cstdint>
#include "../../platform/Mesh.h"
#include "../../platform/Texture.h"
#include "../../platform/MeshData.h"
namespace engine {

struct MeshComponent {
    std::uint32_t entity_id = 0;
    MeshHandle mesh;
};

}  // namespace engine
