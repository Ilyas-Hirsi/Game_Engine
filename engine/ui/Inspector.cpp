#include "Inspector.h"

#include <imgui.h>

#include <glm/gtc/quaternion.hpp>
#include <cstdio>
#include <string>
#include <tuple>

#include "../Scene/Components/Reflection.h"
#include "../Scene/Scene.h"
#include "../assets/AssetRegistry.h"

namespace engine {

namespace {

// A component field of an unhandled type is a compile error 
struct InspectVisitor {
  const AssetRegistry& assets;

  void operator()(const char* name, bool& v) const { ImGui::Checkbox(name, &v); }
  void operator()(const char* name, int& v) const { ImGui::DragInt(name, &v); }
  void operator()(const char* name, float& v) const {
    ImGui::DragFloat(name, &v, 0.05f);
  }
  void operator()(const char* name, glm::vec3& v) const {
    ImGui::DragFloat3(name, &v.x, 0.05f);
  }

  // Edited as degrees so the widget is usable; the quaternion stays normalized.
  void operator()(const char* name, glm::quat& v) const {
    glm::vec3 euler = glm::degrees(glm::eulerAngles(v));
    if (ImGui::DragFloat3(name, &euler.x, 0.5f)) {
      v = glm::normalize(glm::quat(glm::radians(euler)));
    }
  }

  void operator()(const char* name, MeshHandle& v) const {
    const std::string& id = assets.MeshName(v);
    ImGui::LabelText(name, "%s", id.empty() ? "<unregistered>" : id.c_str());
  }
  void operator()(const char* name, TextureHandle& v) const {
    const std::string& id = assets.TextureName(v);
    ImGui::LabelText(name, "%s", id.empty() ? "<none>" : id.c_str());
  }

  void operator()(const char* name, KeyCode& v) const {
    ImGui::LabelText(name, "%d", static_cast<int>(v));
  }

  void operator()(const char* name, std::vector<ChildShape>& v) const {
    ImGui::LabelText(name, "%d shapes", static_cast<int>(v.size()));
  }
};

template <class T>
void InspectComponent(Scene& scene, entity_t entity, const AssetRegistry& assets) {
  T* component = scene.pool<T>().try_get(entity);
  if (component == nullptr) return;

  if (ImGui::CollapsingHeader(Reflect<T>::kName, ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::PushID(Reflect<T>::kName);
    Reflect<T>::Fields(*component, InspectVisitor{assets});
    ImGui::PopID();
  }
}

// The tuple pointer is never dereferenced; it only carries the type list.
template <class... Cs>
void InspectAll(Scene& scene, entity_t entity, const AssetRegistry& assets,
                std::tuple<Cs...>*) {
  (InspectComponent<Cs>(scene, entity, assets), ...);
}

}  // namespace

void DrawInspector(Scene& scene, const AssetRegistry& assets, entity_t& selected) {
  ImGui::Begin("Inspector");

  const std::vector<entity_t>& entities =
      scene.pool<TransformComponent>().dense_entities;

  ImGui::Text("%d entities", static_cast<int>(entities.size()));
  if (ImGui::BeginChild("entities", ImVec2(0.0f, 150.0f), true)) {
    // Clipped so a few thousand entities still cost one screenful of work.
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(entities.size()));
    while (clipper.Step()) {
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
        const entity_t entity = entities[i];
        char label[32];
        std::snprintf(label, sizeof(label), "Entity %u", index_of(entity));
        if (ImGui::Selectable(label, entity == selected)) selected = entity;
      }
    }
  }
  ImGui::EndChild();

  ImGui::Separator();

  if (scene.GetEntities().valid(selected)) {
    InspectAll(scene, selected, assets,
               static_cast<Scene::Registry::component_list*>(nullptr));
  } else {
    ImGui::TextUnformatted("No entity selected");
  }

  ImGui::End();
}

}  // namespace engine
