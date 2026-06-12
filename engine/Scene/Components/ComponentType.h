#pragma once

#include <cstddef>

namespace engine {

enum class ComponentType {
  Transform = 0,
  Sprite,
//   Camera,
  Input,
  Count
};

inline constexpr std::size_t ComponentTypeCount =
    static_cast<std::size_t>(ComponentType::Count);

}