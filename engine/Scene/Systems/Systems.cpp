#include "System.h"
#include "../Components/SpriteComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/MeshComponent.h"
#include "../Entity.h"
#include "../Scene.h"
#include "../../platform/Renderer.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
namespace engine {

void System::HandleInput(Scene& scene, Input& input, float delta_time) {
  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<InputComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }
    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);
    InputComponent& input_component = scene.getComponent<InputComponent>(entity);
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
  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<TransformComponent>(entity)) {
      continue;
    }

    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);
    transform.x += transform.velocity_x * delta_time;
    transform.y += transform.velocity_y * delta_time;
  }
}

void System::Render(Scene& scene, Renderer& renderer) {
  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<SpriteComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }

    SpriteComponent& sprite = scene.getComponent<SpriteComponent>(entity);
    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);
    renderer.DrawSprite(sprite.texture, transform.x, transform.y, sprite.width, sprite.height);
  }

  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<MeshComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }

    MeshComponent& mesh = scene.getComponent<MeshComponent>(entity);
    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f), glm::vec3(transform.x, transform.y, transform.z));
    model = glm::rotate(model, glm::radians(transform.rotation_x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation_y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation_z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(transform.scale_x, transform.scale_y, transform.scale_z));

    renderer.DrawMesh(mesh.mesh, mesh.texture, model);
  }
}

}  // namespace engine
