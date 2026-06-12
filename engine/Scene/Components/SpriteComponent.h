#pragma once
#include <cstdint>
#include "../../platform/Texture.h"
namespace engine
{
    struct SpriteComponent {
        std::uint32_t entity_id;
        TextureHandle texture;
        int width;
        int height;
        int rotation;
        int scale_x;
        int scale_y;
        int color_r;
    };
}
