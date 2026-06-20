#pragma once
#include "../../platform/Input/Input.h"
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
};


}  // namespace engine
