#pragma once

#include <string>
#include <vector>

namespace engine {
class AssetRegistry;

class AssetBrowser {
 public:
  void DrawAssetBrowser(AssetRegistry& assets);

 private:
  enum class PendingImport { None, Texture, Mesh };

  // The registry only knows about assets something has asked for, so the files
  // on disk are listed separately and loaded on demand.
  void Rescan(const AssetRegistry& assets);

  PendingImport pending_ = PendingImport::None;
  std::vector<std::string> texture_files_;
  std::vector<std::string> mesh_files_;
  bool scanned_ = false;
};
}  // namespace engine