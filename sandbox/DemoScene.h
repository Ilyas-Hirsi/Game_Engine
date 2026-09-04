#pragma once

namespace engine {
class AssetRegistry;
class Scene;
}  // namespace engine

void RegisterDemoAssets(engine::AssetRegistry& assets);
void BuildDemoScene(engine::Scene& scene, engine::AssetRegistry& assets);
