#include "AssetBrowser.h"

#include <ImGuiFileDialog.h>
#include <imgui.h>

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

}  // namespace

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

  // Mesh files need a loader before the registry can accept them.
  ImGui::BeginDisabled(true);
  ImGui::Button("Import Mesh...");
  ImGui::EndDisabled();
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("No mesh file loader yet");
  }

  ImGui::Separator();
  ImGui::TextUnformatted(assets.Root().c_str());

  if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const auto& [id, handle] : assets.GetTextures()) {
      ImGui::BulletText("%s", id.c_str());
    }
  }
  if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
    for (const auto& [id, handle] : assets.GetMeshes()) {
      ImGui::BulletText("%s", id.c_str());
    }
  }

  ImGui::End();

  // The dialog is its own top-level window, so it is submitted outside the
  // panel. A minimum size is required: it lays its content out in a zero-sized
  // child, which resolves to nothing unless the window has room to begin with.
  if (ImGuiFileDialog::Instance()->Display(
          "ImportAsset", ImGuiWindowFlags_NoCollapse, ImVec2(700.0f, 400.0f))) {
    if (ImGuiFileDialog::Instance()->IsOk() && pending_ == PendingImport::Texture) {
      const std::string id = ImportInto(
          assets, ImGuiFileDialog::Instance()->GetFilePathName(), "textures");
      // Loading here is safe: the GL context is current inside the ImGui frame.
      if (!id.empty()) assets.Texture(id);
    }
    ImGuiFileDialog::Instance()->Close();
    pending_ = PendingImport::None;
  }
}

}  // namespace engine
