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

    TransformComponent& Scene::AddTransformComponent(Entity& entity, const TransformComponent& transform) {
        const int component_index = static_cast<int>(transform_components_.Size());
        TransformComponent& component = transform_components_.Add(transform);
        component.entity_id = entity.GetId();
        entity.AddComponent(ComponentType::Transform, component_index);
        return component;
    }

    SpriteComponent& Scene::AddSpriteComponent(Entity& entity, const SpriteComponent& sprite) {
        const int component_index = static_cast<int>(sprite_components_.Size());
        SpriteComponent& component = sprite_components_.Add(sprite);
        component.entity_id = entity.GetId();
        entity.AddComponent(ComponentType::Sprite, component_index);
        return component;
    }

    InputComponent& Scene::AddInputComponent(Entity& entity, const InputComponent& input) {
        const int component_index = static_cast<int>(input_components_.Size());
        InputComponent& component = input_components_.Add(input);
        component.entity_id = entity.GetId();
        entity.AddComponent(ComponentType::Input, component_index);
        return component;
    }

    System& Scene::GetSystem() { return system_; }

    std::vector<Entity>& Scene::GetEntities() { return entities_; }

    const std::vector<Entity>& Scene::GetEntities() const { return entities_; }

    std::vector<TransformComponent>& Scene::GetTransformComponents() {
        return transform_components_.All();
    }

    const std::vector<TransformComponent>& Scene::GetTransformComponents() const {
        return transform_components_.All();
    }

    std::vector<SpriteComponent>& Scene::GetSpriteComponents() {
        return sprite_components_.All();
    }

    const std::vector<SpriteComponent>& Scene::GetSpriteComponents() const {
        return sprite_components_.All();
    }
    const std::vector<InputComponent>& Scene::GetInputComponents() const {
        return input_components_.All();
    }
    std::vector<InputComponent>& Scene::GetInputComponents() {
        return input_components_.All();
    }

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
        system_.Render(*this, renderer);
    }
    

}