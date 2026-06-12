#pragma once

#include <cstdint>

namespace engine {

struct TextureHandle {
  std::uint32_t id = 0;

  bool IsValid() const { return id != 0; }
};

}  // namespace engine
