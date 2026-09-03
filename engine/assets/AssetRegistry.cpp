#include "AssetRegistry.h"

#include "../core/Log.h"
#include "../platform/Renderer.h"

namespace engine {

namespace {
const std::string kNoName;

bool IsAbsolutePath(const std::string& path) {
  if (!path.empty() && (path[0] == '/' || path[0] == '\\')) return true;
  return path.size() > 1 && path[1] == ':';
}
}  // namespace

AssetRegistry::AssetRegistry(Renderer& renderer, std::string asset_root)
    : renderer_(renderer), asset_root_(std::move(asset_root)) {}

void AssetRegistry::RegisterMeshBuilder(std::string id,
                                        std::function<MeshData()> builder) {
  mesh_builders_[std::move(id)] = std::move(builder);
}

MeshHandle AssetRegistry::Mesh(const std::string& id) {
  const auto cached = meshes_.find(id);
  if (cached != meshes_.end()) return cached->second;

  const auto builder = mesh_builders_.find(id);
  if (builder == mesh_builders_.end()) {
    LogError("No mesh registered under id: " + id);
    return {};
  }

  const MeshHandle handle = renderer_.CreateMesh(builder->second());
  if (!handle.IsValid()) {
    LogError("Failed to create mesh: " + id);
    return {};
  }

  meshes_[id] = handle;
  mesh_names_[handle.id] = id;
  return handle;
}

TextureHandle AssetRegistry::Texture(const std::string& id) {
  const auto cached = textures_.find(id);
  if (cached != textures_.end()) return cached->second;

  const TextureHandle handle = renderer_.LoadTexture(ResolvePath(id));
  if (!handle.IsValid()) {
    LogError("Failed to load texture: " + id);
    return {};
  }

  textures_[id] = handle;
  texture_names_[handle.id] = id;
  return handle;
}

const std::string& AssetRegistry::MeshName(MeshHandle handle) const {
  const auto found = mesh_names_.find(handle.id);
  return found == mesh_names_.end() ? kNoName : found->second;
}

const std::string& AssetRegistry::TextureName(TextureHandle handle) const {
  const auto found = texture_names_.find(handle.id);
  return found == texture_names_.end() ? kNoName : found->second;
}

std::string AssetRegistry::ResolvePath(const std::string& id) const {
  // Absolute ids pass through so paths outside the asset tree still load.
  if (IsAbsolutePath(id) || asset_root_.empty()) return id;
  return asset_root_ + "/" + id;
}

}  // namespace engine
