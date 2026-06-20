// engine/platform/MeshData.h
#pragma once
#include <cstdint>
#include <vector>

namespace engine {

struct VertexAttribute {
  std::uint32_t location;
  int component_count;
};

struct MeshData {
  std::vector<float> vertices;
  std::vector<unsigned int> indices;
  std::vector<VertexAttribute> layout;

  int Stride() const {
    int s = 0;
    for (const auto& a : layout) s += a.component_count;
    return s;
  }
};

}  // namespace engine