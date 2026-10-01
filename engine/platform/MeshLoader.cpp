#include "MeshLoader.h"

#include <tiny_obj_loader.h>

#include <cmath>
#include <map>
#include <tuple>

#include "../core/Log.h"

namespace engine {

MeshData LoadObj(const std::string& path) {
  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(path) || !reader.Valid()) {
    LogError("Failed to load OBJ " + path + ": " + reader.Error());
    return {};
  }
  if (!reader.Warning().empty()) LogWarn("OBJ " + path + ": " + reader.Warning());

  const tinyobj::attrib_t& attrib = reader.GetAttrib();

  MeshData mesh;
  mesh.layout = {{0, 3}, {1, 3}, {6, 2}};

  // OBJ indexes position, normal and uv separately, so each distinct
  // combination becomes one vertex in the single index buffer GL needs.
  std::map<std::tuple<int, int, int>, unsigned int> emitted;

  for (const tinyobj::shape_t& shape : reader.GetShapes()) {
    for (const tinyobj::index_t& index : shape.mesh.indices) {
      const auto key = std::make_tuple(index.vertex_index, index.normal_index,
                                       index.texcoord_index);
      const auto found = emitted.find(key);
      if (found != emitted.end()) {
        mesh.indices.push_back(found->second);
        continue;
      }

      const int position = 3 * index.vertex_index;
      mesh.vertices.push_back(attrib.vertices[position + 0]);
      mesh.vertices.push_back(attrib.vertices[position + 1]);
      mesh.vertices.push_back(attrib.vertices[position + 2]);

      // Temp lighting until lighting system is implemented
      float shade = 1.0f;
      if (index.normal_index >= 0) {
        const int normal = 3 * index.normal_index;
        const float x = attrib.normals[normal + 0];
        const float y = attrib.normals[normal + 1];
        const float z = attrib.normals[normal + 2];
        const float length_squared = x * x + y * y + z * z;
        if (length_squared > 0.0f) {
          const float up = y / std::sqrt(length_squared);
          shade = 0.4f + 0.6f * (up > 0.0f ? up : 0.0f);
        }
      }
      mesh.vertices.push_back(shade);
      mesh.vertices.push_back(shade);
      mesh.vertices.push_back(shade);

      if (index.texcoord_index >= 0) {
        const int uv = 2 * index.texcoord_index;
        mesh.vertices.push_back(attrib.texcoords[uv + 0]);
        mesh.vertices.push_back(attrib.texcoords[uv + 1]);
      } else {
        mesh.vertices.push_back(0.0f);
        mesh.vertices.push_back(0.0f);
      }

      const auto next = static_cast<unsigned int>(emitted.size());
      emitted.emplace(key, next);
      mesh.indices.push_back(next);
    }
  }

  return mesh;
}

}  // namespace engine
