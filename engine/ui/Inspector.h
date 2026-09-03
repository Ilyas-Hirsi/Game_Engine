#pragma once

#include "../Scene/Entity.h"

namespace engine {

class Scene;
class AssetRegistry;


void DrawInspector(Scene& scene, const AssetRegistry& assets, entity_t& selected);

}  // namespace engine
