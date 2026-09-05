#pragma once

namespace engine {
class AssetRegistry;

class AssetBrowser {
 public:
  void DrawAssetBrowser(AssetRegistry& assets);

 private:
  enum class PendingImport { None, Texture, Mesh };
  PendingImport pending_ = PendingImport::None;
};
}  // namespace engine