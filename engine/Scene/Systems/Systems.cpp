#include "System.h"
#include "../Components/ComponentType.h"
#include "../Components/SpriteComponent.h"
#include "../Components/TransformComponent.h"
#include "../Entity.h"
#include "../Scene.h"
#include "../../platform/Renderer.h"
#include <algorithm>
namespace engine {

void System::HandleInput(Scene& scene, Input& input, float delta_time) {
  for (auto& entity : scene.GetEntities()) {
    if (!entity.HasComponent(ComponentType::Input) ||
        !entity.HasComponent(ComponentType::Transform)) {
      continue;
    }
    TransformComponent& transform = scene.GetTransformComponents()[entity.GetComponentIndex(ComponentType::Transform)];
    InputComponent& input_component = scene.GetInputComponents()[entity.GetComponentIndex(ComponentType::Input)];
    if (input.IsKeyDown(input_component.up)) {
      transform.velocity_y = std::clamp(
          transform.velocity_y - transform.acceleration_y * delta_time,
          -transform.max_velocity_y,
          transform.max_velocity_y);
    } else if (input.IsKeyDown(input_component.down)) {
      transform.velocity_y = std::clamp(
          transform.velocity_y + transform.acceleration_y * delta_time,
          -transform.max_velocity_y,
          transform.max_velocity_y);
    } else {
      transform.velocity_y = 0.0f;
    }

    if (input.IsKeyDown(input_component.left)) {
      transform.velocity_x = std::clamp(
          transform.velocity_x - transform.acceleration_x * delta_time,
          -transform.max_velocity_x,
          transform.max_velocity_x);
    } else if (input.IsKeyDown(input_component.right)) {
      transform.velocity_x = std::clamp(
          transform.velocity_x + transform.acceleration_x * delta_time,
          -transform.max_velocity_x,
          transform.max_velocity_x);
    } else {
      transform.velocity_x = 0.0f;
    }
  }
}
void System::Update(Scene& scene, float delta_time) {
  auto& transform_components = scene.GetTransformComponents();

  for (auto& entity : scene.GetEntities()) {
    if (!entity.HasComponent(ComponentType::Transform)) {
      continue;
    }

    TransformComponent& transform =
        transform_components[entity.GetComponentIndex(ComponentType::Transform)];
    transform.x += transform.velocity_x * delta_time;
    transform.y += transform.velocity_y * delta_time;
  }
}

void System::Render(Scene& scene, Renderer& renderer) {
  auto& sprite_components = scene.GetSpriteComponents();
  auto& transform_components = scene.GetTransformComponents();

  for (auto& entity : scene.GetEntities()) {
    if (!entity.HasComponent(ComponentType::Sprite) || 
    !entity.HasComponent(ComponentType::Transform)) {
      continue;
    }

    SpriteComponent& sprite =
        sprite_components[entity.GetComponentIndex(ComponentType::Sprite)];
    TransformComponent& transform =
        transform_components[entity.GetComponentIndex(ComponentType::Transform)];
    renderer.DrawSprite(sprite.texture, transform.x, transform.y, sprite.width, sprite.height);
  }
}

}  // namespace engine
