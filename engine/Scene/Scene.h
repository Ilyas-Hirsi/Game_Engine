#pragma once
#include <vector>
#include <optional>
#include <utility>
#include "Entity.h"
#include "Systems/System.h"
#include "Components/ComponentStorage.h"
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "../platform/Renderer.h"
#include "Components/InputComponent.h"
#include "Registery.h"
#include "Components/CameraComponent.h"
#include "Camera.h"
#include "Components/PhysicsComponent.h"
#include "Components/TextureComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/ColliderComponent.h"
#include "Components/NameComponent.h"
#include "../Events/EventBus.h"
#include "../core/TaskScheduler.h"
namespace engine {
    class PhysicsSettings {
        public:
        glm::vec3 gravity = glm::vec3(0.0f, -40.81f, 0.0f);
        float fixed_dt = 1.0f / 60.0f;
        float restitution = 0.2f;
    };


    class Scene {

        public:
        using Registry = ECSRegistry<InputComponent, TransformComponent, TextureComponent,
        MeshComponent, CameraComponent, RigidBodyComponent, SpriteComponent,
        ColliderComponent, MovementComponent, NameComponent>;

        Scene(TaskScheduler& task_scheduler);
        virtual ~Scene();
        Entity CreateEntity(std::string name = {});
        void DestroyEntity(Entity entity);
        System& GetSystem();
        EventBus& GetEventBus();
        EntityStorage& GetEntities();
        const EntityStorage& GetEntities() const;
        void HandleInput(Input& input, float deltaTime);
        void Update(float deltaTime);
        void FixedUpdate(float fixed_dt);
        void Render(Renderer& renderer, float alpha = 0.0f);
        RayHit Raycast(const Ray& ray);
        // Empty when no camera is marked active.
        std::optional<CameraMatrices> ActiveCameraMatrices(float aspect);
        PhysicsSettings& GetPhysicsSettings() {return physics_settings_;};
        const PhysicsSettings& GetPhysicsSettings() const {return physics_settings_;};
        std::vector<entity_t>& GetCollidersToCreate() {return colliders_to_create;};
        std::vector<entity_t>& GetCollidersToDestroy() {return colliders_to_destroy;};
        TaskScheduler& GetTaskScheduler() {return task_scheduler_;};
        template <typename T, typename... Args>
        T& emplace(const Entity& ent, Args&&... args){
            if constexpr (std::is_same_v<T, ColliderComponent>){
                colliders_to_create.push_back(ent.GetId());
            }
            return registry_.emplace<T>(ent, std::forward<Args>(args)...);
        }
        template <typename T>
        void remove(const Entity& ent) {
            if constexpr (std::is_same_v<T, ColliderComponent>) {
                if (registry_.has<T>(ent)) colliders_to_destroy.push_back(ent.GetId());
            }
            registry_.remove<T>(ent);
        }
        template <typename T>
        SparseSet<T>& pool(){
            return registry_.pool<T>();
        }
        template <typename T>
        bool has(const Entity& ent){
            return registry_.has<T>(ent);
        }
        template <typename T>
        T& getComponent(const Entity& ent){
            return registry_.getComponent<T>(ent);
        }
        template <typename... Components>
            View<Components...> view() {
                return registry_.view<Components...>();
            }
        protected:
        virtual void OnUpdate(float deltaTime);
        virtual void OnRender(Renderer& renderer, float alpha = 0.0f);

        private:
        System system_;
        EntityStorage entities_;
        Registry registry_;
        PhysicsSettings physics_settings_;
        EventBus event_bus_;
        std::vector<entity_t> colliders_to_create;
        std::vector<entity_t> colliders_to_destroy;
        TaskScheduler& task_scheduler_;
    };
}