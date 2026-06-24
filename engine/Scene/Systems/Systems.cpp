#include "System.h"
#include "../Components/SpriteComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/PhysicsComponent.h"
#include "../Entity.h"
#include "../Scene.h"
#include "../../platform/Renderer.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>
namespace engine {

void System::HandleInput(Scene& scene, Input& input, float delta_time) {
  (void)delta_time;

  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<InputComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }

    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);
    const InputComponent& keys = scene.getComponent<InputComponent>(entity);
    const float turn_speed = 90.0f;
    if (input.IsKeyDown(keys.rotate_left))  transform.rotation.y += turn_speed * delta_time;
    if (input.IsKeyDown(keys.rotate_right)) transform.rotation.y -= turn_speed * delta_time;
    const float yaw = glm::radians(transform.rotation.y);
    transform.facing = glm::vec3(std::sin(yaw), 0.0f, -std::cos(yaw));
    glm::vec3 local(0.0f);
    if (input.IsKeyDown(keys.up)) {
      local.z += 1.0f;
    }
    if (input.IsKeyDown(keys.down)) {
      local.z -= 1.0f;
    }
    if (input.IsKeyDown(keys.right)) {
      local.x += 1.0f;
    }
    if (input.IsKeyDown(keys.left)) {
      local.x -= 1.0f;
    }

    if (glm::dot(local, local) > 0.0f) {
      local = glm::normalize(local);
      const glm::vec3 forward = transform.facing;
      const glm::vec3 right = glm::normalize(glm::cross(transform.facing, glm::vec3(0.0f, 1.0f, 0.0f)));
      transform.move_direction = forward * local.z + right * local.x;
    } else {
      transform.move_direction = glm::vec3(0.0f);
    }
  }
}
void System::Update(Scene& scene, float delta_time) {
  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<TransformComponent>(entity)) {
      continue;
    }
    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);
    transform.position += transform.move_direction * transform.velocity * delta_time;
  }
}

void System::UpdateCamera(Scene& scene, Renderer& renderer) {
  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<CameraComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }

    const CameraComponent& camera = scene.getComponent<CameraComponent>(entity);
    const TransformComponent& transform =
        scene.getComponent<TransformComponent>(entity);

    const float aspect = renderer.GetAspectRatio();
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.fov), aspect, camera.near_plane, camera.far_plane);
    const glm::mat4 view = glm::lookAt(
        transform.position,
        transform.position + transform.facing,
        glm::vec3(0.0f, 1.0f, 0.0f));

    renderer.SetCamera(view, projection);
    return;
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
    renderer.DrawSprite(sprite.texture, transform.position.x, transform.position.y, sprite.width, sprite.height);
  }

  for (auto& entity : scene.GetEntities()) {
    if (!scene.has<MeshComponent>(entity) ||
        !scene.has<TransformComponent>(entity)) {
      continue;
    }

    MeshComponent& mesh = scene.getComponent<MeshComponent>(entity);
    TransformComponent& transform = scene.getComponent<TransformComponent>(entity);

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f), transform.position);
    model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, transform.scale);

    renderer.DrawMesh(mesh.mesh, mesh.texture, model);
  }
}

}  // namespace engine
