#pragma once
#include "../../platform/Input/Input.h"
#include "../Collision.h"
#include "../Entity.h"
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
namespace engine {

class Renderer;
class Scene;
struct SweepEntry {
  entity_t entity;
  AABB     aabb;
};

class System {
 public:
   ~System() = default;
   void Update(Scene& scene, float delta_time);
   void FixedUpdate(Scene& scene, float fixed_dt);
   void Collision(Scene& scene, float delta_time);
   void HandleInput(Scene& scene, Input& input, float delta_time);
   void Render(Scene& scene, Renderer& renderer, float alpha = 0.0f);
   void UpdateCamera(Scene& scene, Renderer& renderer);
  private:
   std::unordered_map<std::uint64_t, std::vector<glm::mat4>> batches;
   std::vector<SweepEntry> sweep_entries_;
   void BuildAABBs(Scene& scene);
   void SortSweepEntries();

   
};


}  // namespace engine
