#include "Scene.h"
#include "Components/TransformComponent.h"
namespace engine {
    Scene::Scene() {

    }
    Scene::~Scene() {
    }

    Entity Scene::CreateEntity() {
        return Entity(entities_.create());
    }
    void Scene::DestroyEntity(Entity& entity) {
        if (!entities_.valid(entity.GetId())) return;
        uint32_t id = entity.GetId();
        registry_.removeAll(id);
        entities_.destroy(id);

    }

    System& Scene::GetSystem() { return system_; }

    EntityStorage& Scene::GetEntities() { return entities_; }

    const EntityStorage& Scene::GetEntities() const { return entities_; }
    

    void Scene::HandleInput(Input& input, float deltaTime) {
        system_.HandleInput(*this, input, deltaTime);
    }

    void Scene::Update(float deltaTime) {
        OnUpdate(deltaTime);
    }
    void Scene::FixedUpdate(float fixed_dt) {
        view<TransformComponent, RigidBodyComponent>().each(
            [](uint32_t, TransformComponent& t, RigidBodyComponent& rb) {
                rb.previous_position = t.position;
            });
        system_.FixedUpdate(*this, fixed_dt);
        system_.Collision(*this, fixed_dt);
    }
    void Scene::Render(Renderer& renderer, float alpha) {
        OnRender(renderer, alpha);
    }
    void Scene::OnUpdate(float deltaTime) {
        // Move everything (input + physics integration), then resolve the
        // overlaps that motion produced.
        system_.Update(*this, deltaTime);
    }

    void Scene::OnRender(Renderer& renderer, float alpha) {
        system_.UpdateCamera(*this, renderer);
        system_.Render(*this, renderer, alpha);
    }
    
}
