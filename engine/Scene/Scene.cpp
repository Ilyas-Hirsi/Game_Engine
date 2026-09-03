#include "Scene.h"
#include "Components/TransformComponent.h"
#include "../Events/CollisionEvents.h"
namespace engine {
    Scene::Scene(TaskScheduler& task_scheduler) : task_scheduler_(task_scheduler) {

    }
    Scene::~Scene() {
    }

    Entity Scene::CreateEntity() {
        return Entity(entities_.create());
    }
    void Scene::DestroyEntity(Entity& entity) {
        if (!entities_.valid(entity.GetId())) return;
        uint32_t id = entity.GetId();
        if (pool<ColliderComponent>().contains(id))
          colliders_to_destroy.push_back(id);
        registry_.removeAll(id);
        entities_.destroy(id);
      }

    System& Scene::GetSystem() { return system_; }

    EventBus& Scene::GetEventBus() { return event_bus_; }

    EntityStorage& Scene::GetEntities() { return entities_; }

    const EntityStorage& Scene::GetEntities() const { return entities_; }
    
    RayHit Scene::Raycast(const Ray& ray) {
        return system_.Raycast(*this, ray);
    }
    

    void Scene::HandleInput(Input& input, float deltaTime) {
        system_.HandleInput(*this, input, deltaTime);
    }

    void Scene::Update(float deltaTime) {
        OnUpdate(deltaTime);
        // After OnUpdate so colliders created this frame reach the trees before
        // FixedUpdate queries them; runs every frame, so pausing cannot stall it.
        system_.SyncColliderProxies(*this);
    }
    void Scene::FixedUpdate(float fixed_dt) {
        system_.FixedUpdate(*this, fixed_dt);
        system_.Collision(*this, fixed_dt);
        event_bus_.Dispatch();
    }
    void Scene::Render(Renderer& renderer, float alpha) {
        OnRender(renderer, alpha);
    }
    std::optional<CameraMatrices> Scene::ActiveCameraMatrices(float aspect) {
        std::optional<CameraMatrices> result;
        view<CameraComponent, TransformComponent, MovementComponent>().each(
            [&](std::uint32_t, CameraComponent& camera, TransformComponent& transform,
                MovementComponent& move) {
                
                if (result || !camera.active) return;
                result = ComputeCamera(camera, transform, move, aspect);
            });
        return result;
    }

    void Scene::OnUpdate(float deltaTime) {

        system_.Update(*this, deltaTime);
    }

    void Scene::OnRender(Renderer& renderer, float alpha) {
        system_.UpdateCamera(*this, renderer);
        system_.Render(*this, renderer, alpha);
    }
    
}
