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
#include "Components/PhysicsComponent.h"
#include "Components/TextureComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/ColliderComponent.h"
namespace engine {
    class PhysicsSettings {
        public:
        glm::vec3 gravity = glm::vec3(0.0f, -40.81f, 0.0f);
        float fixed_dt = 1.0f / 60.0f;
        float restitution = 0.2f;
    };

    class Scene {

        public:
        Scene();
        virtual ~Scene();
        Entity CreateEntity();
        void DestroyEntity(Entity& entity);
        System& GetSystem();
        EntityStorage& GetEntities();
        const EntityStorage& GetEntities() const;
        void HandleInput(Input& input, float deltaTime);
        void Update(float deltaTime);
        void FixedUpdate(float fixed_dt);
        void Render(Renderer& renderer, float alpha = 0.0f);
        PhysicsSettings& GetPhysicsSettings() {return physics_settings_;};
        const PhysicsSettings& GetPhysicsSettings() const {return physics_settings_;};
        template <typename T, typename... Args>
        T& emplace(const Entity& ent, Args&&... args){
            return registry_.emplace<T>(ent, std::forward<Args>(args)...);
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
        std::uint32_t next_entity_id_ = 0;
        EntityStorage entities_;
        ECSRegistry<InputComponent, TransformComponent, TextureComponent,
        MeshComponent, CameraComponent, RigidBodyComponent, SpriteComponent,
        ColliderComponent, MovementComponent> registry_;
        PhysicsSettings physics_settings_;
    };
}