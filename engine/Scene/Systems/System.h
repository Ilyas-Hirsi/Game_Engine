#pragma once
#include "../../platform/Input/Input.h"
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
namespace engine {

class Renderer;
class Scene;

class System {
 public:
   ~System() = default;
   void Update(Scene& scene, float delta_time);
   void HandleInput(Scene& scene, Input& input, float delta_time);
   void Render(Scene& scene, Renderer& renderer);
   void UpdateCamera(Scene& scene, Renderer& renderer);
  private:
   std::unordered_map<std::uint64_t, std::vector<glm::mat4>> batches;
};


}  // namespace engine
