#include "System.h"
#include "../Components/SpriteComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/PhysicsComponent.h"
#include "../Components/TextureComponent.h"
#include "../Components/ColliderComponent.h"
#include "../Entity.h"
#include "../Scene.h"
#include "../Collision.h"
#include "../../platform/Renderer.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>
namespace engine {

void System::HandleInput(Scene& scene, Input& input, float delta_time) {
  (void)delta_time;

  scene.view<InputComponent, TransformComponent, MovementComponent>().each(
      [&](uint32_t, InputComponent& keys, TransformComponent& transform, MovementComponent& move) {
    const float turn_speed = 90.0f;
    if (input.IsKeyDown(keys.rotate_left))  transform.rotation.y += turn_speed * delta_time;
    if (input.IsKeyDown(keys.rotate_right)) transform.rotation.y -= turn_speed * delta_time;
    if (input.IsKeyDown(keys.rotate_up)) transform.rotation.x += turn_speed * delta_time;
    if (input.IsKeyDown(keys.rotate_down)) transform.rotation.x -= turn_speed * delta_time;
    if (input.IsKeyDown(keys.camera_up)) transform.position.y += move.speed * delta_time;
    if (input.IsKeyDown(keys.camera_down)) transform.position.y -= move.speed * delta_time;
    const float yaw = glm::radians(transform.rotation.y);
    const float pitch = glm::radians(transform.rotation.x);
    move.facing = glm::vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
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
      const glm::vec3 forward = move.facing;
      const glm::vec3 right = glm::normalize(glm::cross(move.facing, glm::vec3(0.0f, 1.0f, 0.0f)));
      move.move_direction = forward * local.z + right * local.x;
    } else {
      move.move_direction = glm::vec3(0.0f);
    }
  });
}
void System::Update(Scene& scene, float delta_time) {
  scene.view<TransformComponent, MovementComponent>().each(
    [&](uint32_t, TransformComponent& t, MovementComponent& m) {

      t.position += m.move_direction * m.speed * delta_time;
    }
  );
}

void System::FixedUpdate(Scene& scene, float fixed_dt) {
    // Manage Gravity
    auto& physics_settings = scene.GetPhysicsSettings();
    scene.view<TransformComponent, RigidBodyComponent>().each(
        [&](uint32_t, TransformComponent& t, RigidBodyComponent& rb) {
            if (rb.inverse_mass == 0.0f) return;
            rb.linear_velocity += physics_settings.gravity * rb.gravity_scale * fixed_dt;
            t.position += rb.linear_velocity * fixed_dt;
        });
}

// Current collision implementation is a simple sweep and prune algorithm
void System::Collision(Scene& scene, float) {
  auto& colliders = scene.pool<ColliderComponent>();
  auto& transforms = scene.pool<TransformComponent>();
  auto& rigid_bodies = scene.pool<RigidBodyComponent>();
  auto& dense = colliders.dense_entities;

  const float default_restitution = 0.6f;

  // Check if there are any memberships issues with the sweep entries
  std::size_t sweepable = 0;
  for (entity_t entity : dense) {
    if (!transforms.contains(entity)) continue;
    if (std::holds_alternative<Plane>(colliders.get(entity).shape)) continue;
    ++sweepable;
  }
  bool rebuild = sweepable != sweep_entries_.size();
  if (!rebuild) {
    for (auto& entry : sweep_entries_) {
      if (!transforms.contains(entry.entity) || !colliders.contains(entry.entity)) {
        rebuild = true;
        break;
      }
      entry.aabb = ComputeAABB(transforms.get(entry.entity).position,
                               colliders.get(entry.entity));
    }
  }

  if (rebuild) {
    BuildAABBs(scene);
  } else {
    SortSweepEntries();
  }

  // resolve the collisions
  auto resolve = [&](entity_t ea, entity_t eb, const Contact& c) {
    RigidBodyComponent* ra = rigid_bodies.try_get(ea);
    RigidBodyComponent* rb = rigid_bodies.try_get(eb);
    const float wa = ra ? ra->inverse_mass : 0.0f;
    const float wb = rb ? rb->inverse_mass : 0.0f;
    const float total = wa + wb;
    if (total == 0.0f) return;  // two statics: nothing to do

    transforms.get(ea).position -= c.normal * c.penetration * (wa / total);
    transforms.get(eb).position += c.normal * c.penetration * (wb / total);

    const glm::vec3 va = ra ? ra->linear_velocity : glm::vec3(0.0f);
    const glm::vec3 vb = rb ? rb->linear_velocity : glm::vec3(0.0f);
    const float vn = glm::dot(vb - va, c.normal);
    if (vn < 0.0f) {
      const float ra_rest = ra ? ra->restitution : default_restitution;
      const float rb_rest = rb ? rb->restitution : default_restitution;
      const float jimp = -(1.0f + 0.5f * (ra_rest + rb_rest)) * vn / total;
      if (ra) ra->linear_velocity -= c.normal * (jimp * wa);
      if (rb) rb->linear_velocity += c.normal * (jimp * wb);
    }
  };

  // sweep the sweep entries
  for (std::size_t i = 0; i < sweep_entries_.size(); ++i) {
    const SweepEntry& a = sweep_entries_[i];
    for (std::size_t j = i + 1; j < sweep_entries_.size(); ++j) {
      const SweepEntry& b = sweep_entries_[j];
      if (b.aabb.min.x > a.aabb.max.x) break;
      if (a.aabb.min.y > b.aabb.max.y || a.aabb.max.y < b.aabb.min.y) continue;
      if (a.aabb.min.z > b.aabb.max.z || a.aabb.max.z < b.aabb.min.z) continue;

      const Contact c = Collide(transforms.get(a.entity).position, colliders.get(a.entity),
                                transforms.get(b.entity).position, colliders.get(b.entity));
      if (c.hit) resolve(a.entity, b.entity, c);
    }
  }

  // handle the plane collisions
  for (entity_t plane_entity : dense) {
    ColliderComponent& plane_collider = colliders.get(plane_entity);
    if (!std::holds_alternative<Plane>(plane_collider.shape)) continue;
    const glm::vec3 plane_pos = transforms.contains(plane_entity)
        ? transforms.get(plane_entity).position : glm::vec3(0.0f);

    for (const SweepEntry& entry : sweep_entries_) {
      const Contact c = Collide(transforms.get(entry.entity).position,
                                colliders.get(entry.entity),
                                plane_pos, plane_collider);
      if (c.hit) resolve(entry.entity, plane_entity, c);
    }
  }
}

void System::UpdateCamera(Scene& scene, Renderer& renderer) {
  bool camera_set = false;
  scene.view<CameraComponent, TransformComponent, MovementComponent>().each(
      [&](uint32_t, CameraComponent& camera, TransformComponent& transform, MovementComponent& move) {
    if (camera_set) return;  // only the first camera drives the view

    const float aspect = renderer.GetAspectRatio();
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.fov), aspect, camera.near_plane, camera.far_plane);
    const glm::mat4 view = glm::lookAt(
        transform.position,
        transform.position + move.facing,
        glm::vec3(0.0f, 1.0f, 0.0f));

    renderer.SetCamera(view, projection);
    camera_set = true;
  });
}

void System::Render(Scene& scene, Renderer& renderer, float alpha) {
  auto& bodies = scene.pool<RigidBodyComponent>();
  scene.view<SpriteComponent, TransformComponent>().each(
    [&](entity_t entity, SpriteComponent& sprite, TransformComponent& transform) {
       RigidBodyComponent* rb = bodies.try_get(entity);
       const glm::vec3 position = rb
        ? glm::mix(rb->previous_position, transform.position, alpha)
        : transform.position;

        renderer.DrawSprite(sprite.texture, position.x, position.y,
                            sprite.width, sprite.height);
    });

    // Batching meshes with the same texture to improve performance
  auto& texture_pool = scene.pool<TextureComponent>();
  scene.view<MeshComponent, TransformComponent>().each(
      [&](uint32_t entity, MeshComponent& mesh, TransformComponent& transform) {
          RigidBodyComponent* rb = bodies.try_get(entity);
          const glm::vec3 position = rb
            ? glm::mix(rb->previous_position, transform.position, alpha)
            : transform.position;
        
          glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
          model = glm::rotate(model, glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
          model = glm::rotate(model, glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
          model = glm::rotate(model, glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
          model = glm::scale(model, transform.scale);
  
          std::uint32_t tex = texture_pool.contains(entity)
              ? texture_pool.get(entity).texture.id : 0;
          std::uint64_t key = (std::uint64_t(mesh.mesh.id) << 32) | tex;
          batches[key].push_back(model);
      });
  
  for (auto& [key, models] : batches) {
      MeshHandle mesh{ std::uint32_t(key >> 32) };
      TextureHandle texture{ std::uint32_t(key & 0xFFFFFFFF) };
      renderer.DrawMeshInstanced(mesh, texture, models);
  }
  batches.reserve(batches.size());
  batches.clear();
  
}

// Full rebuild of the sweep array: only for startup and frames where a
// collider was added/removed. Steady-state frames refresh in place and
// insertion-sort instead, to keep the near-sorted order cheap to maintain.
void System::BuildAABBs(Scene& scene) {
  auto& colliders  = scene.pool<ColliderComponent>();
  auto& transforms = scene.pool<TransformComponent>();

  sweep_entries_.clear();
  for (entity_t entity : colliders.dense_entities) {
    if (!transforms.contains(entity)) continue;
    const ColliderComponent& collider = colliders.get(entity);
    if (std::holds_alternative<Plane>(collider.shape)) continue;  // planes get their own pass
    sweep_entries_.push_back(
        {entity, ComputeAABB(transforms.get(entity).position, collider)});
  }

  std::sort(sweep_entries_.begin(), sweep_entries_.end(),
            [](const SweepEntry& a, const SweepEntry& b) {
              return a.aabb.min.x < b.aabb.min.x;
            });
}

// Insertion sort by min.x: bodies barely move between fixed steps, so the
// array is nearly sorted and this runs in ~O(n + swaps).
void System::SortSweepEntries() {
  for (std::size_t i = 1; i < sweep_entries_.size(); ++i) {
    SweepEntry entry = sweep_entries_[i];
    std::size_t j = i;
    while (j > 0 && sweep_entries_[j - 1].aabb.min.x > entry.aabb.min.x) {
      sweep_entries_[j] = sweep_entries_[j - 1];
      --j;
    }
    sweep_entries_[j] = entry;
  }
}
}  // namespace engine
