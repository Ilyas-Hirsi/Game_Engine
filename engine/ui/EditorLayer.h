#pragma once

#include <limits>
#include <vector>

#include "../Scene/Entity.h"

namespace engine {

class AssetRegistry;
class Input;
class Renderer;
class Scene;
class Window;

// Owns the editor's own state: what is selected and which panels are open.
class EditorLayer {
 public:
  // Entity 0 is a real entity, so "nothing picked" needs an id that never is.
  static constexpr entity_t kNoSelection = std::numeric_limits<entity_t>::max();

  void Draw(Scene& scene, AssetRegistry& assets, bool& paused);
  void HandlePicking(Scene& scene, Input& input, Renderer& renderer, Window& window);

  entity_t Selected() const { return selected_; }
  void SetSelected(entity_t entity) { selected_ = entity; }

 private:
  void DrawMenuBar(Scene& scene);
  void DrawStats(Scene& scene, bool& paused);

  entity_t selected_ = kNoSelection;
  std::vector<entity_t> scratch_;
  bool show_hierarchy_ = true;
  bool show_inspector_ = true;
  bool show_stats_ = true;
};

}  // namespace engine
