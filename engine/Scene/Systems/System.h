#pragma once
#include "../../platform/Input/Input.h"
#include "../Collision.h"
#include "../Entity.h"
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "../AABBTree.h"
namespace engine {

class Renderer;
class Scene;

class System {
 public:
   ~System() = default;
   void Update(Scene& scene, float delta_time);
   void FixedUpdate(Scene& scene, float fixed_dt);
   void Collision(Scene& scene, float delta_time);
   void SyncColliderProxies(Scene& scene);
   RayHit Raycast(Scene& scene, const Ray& query);
   void HandleInput(Scene& scene, Input& input, float delta_time);
   void Render(Scene& scene, Renderer& renderer, float alpha = 0.0f);
   void UpdateCamera(Scene& scene, Renderer& renderer);
  private:
   struct PairContact { entity_t a; entity_t b; ContactManifold manifold; };

   std::unordered_map<std::uint64_t, std::vector<glm::mat4>> batches;
   BVHTree dynamic_tree_;
   BVHTree static_tree_;
   std::unordered_map<entity_t, int> dynamic_proxies_;
   std::unordered_map<entity_t, int> static_proxies_;
   std::vector<entity_t> plane_entities_;

   std::vector<entity_t> dynamic_entities_;
   std::vector<AABB> dynamic_boxes_;
   std::vector<std::vector<entity_t>> candidates_;
   std::vector<std::vector<PairContact>> pair_contacts_;
};


}  // namespace engine
