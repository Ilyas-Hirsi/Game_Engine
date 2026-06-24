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
namespace engine {

    class Scene {

        public:
        Scene();
        virtual ~Scene();
        Entity& CreateEntity();
        System& GetSystem();
        std::vector<Entity>& GetEntities();
        const std::vector<Entity>& GetEntities() const;
        void HandleInput(Input& input, float deltaTime);
        void Update(float deltaTime);
        void Render(Renderer& renderer);
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
        virtual void OnRender(Renderer& renderer);

        private:
        System system_;
        std::uint32_t next_entity_id_ = 0;
        std::vector<Entity> entities_;
         ECSRegistry<InputComponent, TransformComponent, TextureComponent,
         MeshComponent, CameraComponent, PhysicsComponent, SpriteComponent> registry_;
    };
}