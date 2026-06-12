#pragma once
#include <string>
#include <vector>
#include "Entity.h"
#include "Systems/System.h"
#include "Components/ComponentStorage.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "../platform/Renderer.h"
#include "Components/InputComponent.h"
namespace engine {

    class Scene {
        public:
        Scene();
        virtual ~Scene();
        Entity& CreateEntity(const std::string& name);
        System& GetSystem();
        TransformComponent& AddTransformComponent(Entity& entity, const TransformComponent& transform);
        SpriteComponent& AddSpriteComponent(Entity& entity, const SpriteComponent& sprite);
        InputComponent& AddInputComponent(Entity& entity, const InputComponent& input);
        std::vector<Entity>& GetEntities();
        const std::vector<Entity>& GetEntities() const;
        std::vector<TransformComponent>& GetTransformComponents();
        const std::vector<TransformComponent>& GetTransformComponents() const;
        std::vector<SpriteComponent>& GetSpriteComponents();
        std::vector<InputComponent>& GetInputComponents();
        const std::vector<InputComponent>& GetInputComponents() const;
        const std::vector<SpriteComponent>& GetSpriteComponents() const;
        void HandleInput(Input& input, float deltaTime);
        void Update(float deltaTime);
        void Render(Renderer& renderer);
        protected:
        virtual void OnUpdate(float deltaTime);
        virtual void OnRender(Renderer& renderer);

        private:
        System system_;
        std::uint32_t next_entity_id_ = 0;
        std::vector<Entity> entities_;
        ComponentStorage<InputComponent> input_components_;
        ComponentStorage<TransformComponent> transform_components_;
        ComponentStorage<SpriteComponent> sprite_components_;

    };
}