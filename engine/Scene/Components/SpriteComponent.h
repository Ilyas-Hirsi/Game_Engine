#pragma once
#include "../../platform/Texture.h"
namespace engine {

// 2D screen-space sprite: its own texture plus the size to draw it at.
// Distinct from TextureComponent (which only marks that a mesh has a texture).
struct SpriteComponent {
    TextureHandle texture;
    int width = 0;
    int height = 0;
};

}  // namespace engine
