#include "Hierarchy.h"
#include <cstdio>
#include <vector>
#include <imgui.h>
#include "../Scene/Components/NameComponent.h"
#include "../Scene/Entity.h"
#include "../Scene/Scene.h"

namespace engine {

    void DrawHierarchy(Scene& scene, entity_t& selected, std::vector<entity_t>& scratch) {
        ImGui::Begin("Hierarchy");
      
        const EntityStorage& entities = scene.GetEntities();
        scratch.clear();
        scratch.reserve(entities.size());
        for (std::size_t i = 0; i < entities.capacity(); ++i) {
          if (entities.slot_alive(i)) scratch.push_back(entities.slot(i));
        }
      
        ImGui::Text("%d entities", static_cast<int>(scratch.size()));
      
        SparseSet<NameComponent>& names = scene.pool<NameComponent>();
        if (ImGui::BeginChild("entities", ImVec2(0.0f, 0.0f), true)) {
          ImGuiListClipper clipper;
          clipper.Begin(static_cast<int>(scratch.size()));
          while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
              const entity_t entity = scratch[i];
              const NameComponent* name = names.try_get(entity);
              char label[64];
              if (name != nullptr && !name->name.empty()) {
                std::snprintf(label, sizeof(label), "%s##%u", name->name.c_str(), entity);
              } else {
                std::snprintf(label, sizeof(label), "Entity %u##%u", index_of(entity), entity);
              }
              if (ImGui::Selectable(label, entity == selected)) selected = entity;
            }
          }
        }
        ImGui::EndChild();
        ImGui::End();
      }
}