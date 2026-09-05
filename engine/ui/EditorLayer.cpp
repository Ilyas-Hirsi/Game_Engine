#include "EditorLayer.h"

#include <imgui.h>

#include "Hierarchy.h"
#include "Inspector.h"
#include "../Scene/Camera.h"
#include "../Scene/Components/TransformComponent.h"
#include "../Scene/Scene.h"
#include "../platform/Input/Input.h"
#include "../platform/Renderer.h"
#include "../platform/Window.h"

namespace engine {

void EditorLayer::Draw(Scene& scene, AssetRegistry& assets, bool& paused) {
  
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),
                               ImGuiDockNodeFlags_PassthruCentralNode);

  DrawMenuBar(scene);
  if (show_stats_) DrawStats(scene, paused);
  if (show_hierarchy_) DrawHierarchy(scene, selected_, scratch_);
  if (show_inspector_) DrawInspector(scene, assets, selected_);
  if (show_asset_browser_) asset_browser_.DrawAssetBrowser(assets);
}

void EditorLayer::DrawMenuBar(Scene& scene) {
  if (!ImGui::BeginMainMenuBar()) return;

  if (ImGui::BeginMenu("Entity")) {
    if (ImGui::MenuItem("Create Empty")) {
      Entity entity = scene.CreateEntity("Entity");
      scene.emplace<TransformComponent>(entity, TransformComponent{});
      selected_ = entity.GetId();
    }
    const bool has_selection = scene.GetEntities().valid(selected_);
    if (ImGui::MenuItem("Delete Selected", nullptr, false, has_selection)) {
      scene.DestroyEntity(Entity(selected_));
      selected_ = kNoSelection;
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    ImGui::MenuItem("Hierarchy", nullptr, &show_hierarchy_);
    ImGui::MenuItem("Inspector", nullptr, &show_inspector_);
    ImGui::MenuItem("Stats", nullptr, &show_stats_);
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void EditorLayer::DrawStats(Scene& scene, bool& paused) {
  ImGui::Begin("Stats", &show_stats_);

  const ImGuiIO& io = ImGui::GetIO();
  ImGui::Text("%.1f FPS (%.2f ms)", io.Framerate, 1000.0f / io.Framerate);
  ImGui::Checkbox("Paused (click to select)", &paused);
  // Writes straight into the live PhysicsSettings the fixed step reads.
  ImGui::DragFloat3("Gravity", &scene.GetPhysicsSettings().gravity.x, 0.5f);

  ImGui::End();
}

void EditorLayer::HandlePicking(Scene& scene, Input& input, Renderer& renderer,
                                Window& window) {
  // Reflects the previous frame, since picking runs before ImGui::NewFrame.
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse) return;
  if (!input.WasMouseButtonPressed(MouseButton::Left)) return;

  std::optional<CameraMatrices> matrices =
      scene.ActiveCameraMatrices(renderer.GetAspectRatio());
  if (!matrices) return;

  const Ray ray = ScreenPointToRay(*matrices, input.MouseX(), input.MouseY(),
                                   window.Width(), window.Height());
  const RayHit hit = scene.Raycast(ray);
  selected_ = hit.Hit() ? hit.entity : kNoSelection;
}

}  // namespace engine
