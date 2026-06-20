#include "Scene.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
namespace engine {
    Scene::Scene() {

    }
    Scene::~Scene() {
    }

    Entity& Scene::CreateEntity(const std::string& name) {
        entities_.emplace_back(next_entity_id_++, name);
        return entities_.back();
    }

    System& Scene::GetSystem() { return system_; }

    std::vector<Entity>& Scene::GetEntities() { return entities_; }

    const std::vector<Entity>& Scene::GetEntities() const { return entities_; }
    

    void Scene::HandleInput(Input& input, float deltaTime) {
        system_.HandleInput(*this, input, deltaTime);
    }

    void Scene::Update(float deltaTime) {
        OnUpdate(deltaTime);
    }
    void Scene::Render(Renderer& renderer) {
        OnRender(renderer);
    }
    void Scene::OnUpdate(float deltaTime) {
        // update all entity positions
        system_.Update(*this, deltaTime);
    }

    void Scene::OnRender(Renderer& renderer) {
        system_.UpdateCamera(*this, renderer);
        system_.Render(*this, renderer);
    }
    

}