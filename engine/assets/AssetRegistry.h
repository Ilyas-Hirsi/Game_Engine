#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "../platform/MeshData.h"
#include "../platform/Mesh.h"
#include "../platform/Texture.h"

namespace engine {

class Renderer;

// Maps asset ids to runtime handles
class AssetRegistry {
 public:
  explicit AssetRegistry(Renderer& renderer, std::string asset_root = "assets");

  AssetRegistry(const AssetRegistry&) = delete;
  AssetRegistry& operator=(const AssetRegistry&) = delete;

  // Handle procedural meshes which have no file
  void RegisterMeshBuilder(std::string id, std::function<MeshData()> builder);

  MeshHandle Mesh(const std::string& id);
  TextureHandle Texture(const std::string& id);

  void UnloadMesh(const std::string& id);
  void UnloadTexture(const std::string& id);

  // Rebuilds in place, so handles already stored in components stay valid.
  bool ReloadMesh(const std::string& id);

  // Empty when handle did not come from registry
  const std::string& MeshName(MeshHandle handle) const;
  const std::string& TextureName(TextureHandle handle) const;
  const std::unordered_map<std::string, MeshHandle>& GetMeshes() const;
  const std::unordered_map<std::string, TextureHandle>& GetTextures() const;

  const std::string& Root() const { return asset_root_; }

 private:
  std::string ResolvePath(const std::string& id) const;

  Renderer& renderer_;
  std::string asset_root_;
  std::unordered_map<std::string, std::function<MeshData()>> mesh_builders_;
  std::unordered_map<std::string, MeshHandle> meshes_;
  std::unordered_map<std::uint32_t, std::string> mesh_names_;
  std::unordered_map<std::string, TextureHandle> textures_;
  std::unordered_map<std::uint32_t, std::string> texture_names_;
};

}  // namespace engine
