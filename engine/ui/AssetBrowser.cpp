#include "AssetBrowser.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string>

#include "../assets/AssetRegistry.h"
#include "../core/Log.h"

namespace engine {

namespace {

namespace fs = std::filesystem;

// Copies the chosen file into the asset root and returns the id it was
// registered under
std::string ImportInto(const AssetRegistry& assets, const fs::path& source,
                       const std::string& folder) {
  std::error_code ec;
  const fs::path dest_dir = fs::path(assets.Root()) / folder;
  fs::create_directories(dest_dir, ec);

  const std::uintmax_t source_size = fs::file_size(source, ec);
  if (ec) {
    LogError("Cannot read import source: " + source.string());
    return {};
  }


  fs::path dest = dest_dir / source.filename();
  for (int suffix = 1; fs::exists(dest); ++suffix) {
    if (fs::file_size(dest, ec) == source_size) {
      return folder + "/" + dest.filename().string();
    }
    dest = dest_dir / (source.stem().string() + "_" + std::to_string(suffix) +
                       source.extension().string());
  }

  fs::copy_file(source, dest, ec);
  if (ec) {
    LogError("Failed to copy asset into " + dest_dir.string() + ": " + ec.message());
    return {};
  }
  return folder + "/" + dest.filename().string();
}

void ListFolder(const std::string& root, const std::string& folder,
                std::vector<std::string>& out) {
  out.clear();
  std::error_code ec;
  const fs::path dir = fs::path(root) / folder;
  for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
    if (!entry.is_regular_file(ec)) continue;
    const std::string name = entry.path().filename().string();
    if (!name.empty() && name.front() == '.') continue;
    out.push_back(folder + "/" + name);
  }
  std::sort(out.begin(), out.end());
}

}  // namespace

void AssetBrowser::Rescan(const AssetRegistry& assets) {
  ListFolder(assets.Root(), "textures", texture_files_);
  ListFolder(assets.Root(), "meshes", mesh_files_);
  scanned_ = true;
}

void AssetBrowser::DrawAssetBrowser(AssetRegistry& assets) {
  ImGui::Begin("Assets");

  IGFD::FileDialogConfig config;
  config.path = ".";

  if (ImGui::Button("Import Texture...")) {
    pending_ = PendingImport::Texture;
    ImGuiFileDialog::Instance()->OpenDialog(
        "ImportAsset", "Import Texture", ".png,.jpg,.jpeg,.bmp,.tga", config);
  }
  ImGui::SameLine();

  if (ImGui::Button("Import Mesh...")) {
    pending_ = PendingImport::Mesh;
    ImGuiFileDialog::Instance()->OpenDialog("ImportAsset", "Import Mesh", ".obj",
                                            config);
  }

  ImGui::SameLine();
  if (ImGui::Button("Refresh") || !scanned_) Rescan(assets);

  ImGui::Separator();
  ImGui::TextUnformatted(assets.Root().c_str());

  // Loading is safe here: the GL context is current inside the ImGui frame.
  if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const std::string& id : texture_files_) {
      const bool loaded = assets.GetTextures().count(id) != 0;
      if (ImGui::Selectable(id.c_str(), loaded) && !loaded) assets.Texture(id);
    }
  }
  if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const std::string& id : mesh_files_) {
      const bool loaded = assets.GetMeshes().count(id) != 0;
      if (ImGui::Selectable(id.c_str(), loaded) && !loaded) assets.Mesh(id);
    }
  }

  ImGui::End();

  // The dialog is its own top-level window, so it is submitted outside the
  // panel. A minimum size is required: it lays its content out in a zero-sized
  // child, which resolves to nothing unless the window has room to begin with.
  if (ImGuiFileDialog::Instance()->Display(
          "ImportAsset", ImGuiWindowFlags_NoCollapse, ImVec2(700.0f, 400.0f))) {
    // Loading here is safe: the GL context is current inside the ImGui frame.
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const fs::path source = ImGuiFileDialog::Instance()->GetFilePathName();
      if (pending_ == PendingImport::Texture) {
        const std::string id = ImportInto(assets, source, "textures");
        if (!id.empty()) { assets.Texture(id); scanned_ = false; }
      } else if (pending_ == PendingImport::Mesh) {
        const std::string id = ImportInto(assets, source, "meshes");
        if (!id.empty()) { assets.Mesh(id); scanned_ = false; }
      }
    }
    ImGuiFileDialog::Instance()->Close();
    pending_ = PendingImport::None;
  }
}

}  // namespace engine
