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
#include "../../Events/EventBus.h"
#include "../../Events/CollisionEvents.h"
namespace engine {

void System::HandleInput(Scene& scene, Input& input, float delta_time) {
  (void)delta_time;

  scene.view<InputComponent, TransformComponent, MovementComponent>().each(
      [&](uint32_t, InputComponent& keys, TransformComponent& transform, MovementComponent& move) {
        const float turn_speed = glm::radians(90.0f);
        if (input.IsKeyDown(keys.rotate_left))  move.yaw += turn_speed * delta_time;
        if (input.IsKeyDown(keys.rotate_right)) move.yaw -= turn_speed * delta_time;
        if (input.IsKeyDown(keys.rotate_up))    move.pitch += turn_speed * delta_time;
        if (input.IsKeyDown(keys.rotate_down))  move.pitch -= turn_speed * delta_time;
        if (input.IsKeyDown(keys.camera_up))   transform.position.y += move.speed * delta_time;
        if (input.IsKeyDown(keys.camera_down)) transform.position.y -= move.speed * delta_time;
        move.pitch = glm::clamp(move.pitch, glm::radians(-89.0f), glm::radians(89.0f));
        transform.rotation_quat =
            glm::angleAxis(move.yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::angleAxis(move.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
        move.facing = transform.rotation_quat * glm::vec3(0.0f, 0.0f, -1.0f);
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
            rb.previous_position = t.position;
            rb.previous_rotation_quat = t.rotation_quat;
            if (rb.inverse_mass == 0.0f) return;
            rb.linear_velocity += physics_settings.gravity * rb.gravity_scale * fixed_dt;
            t.position += rb.linear_velocity * fixed_dt;
            rb.angular_velocity += rb.angular_acceleration * fixed_dt;
            glm::quat omega_quat(0.0f, rb.angular_velocity.x, rb.angular_velocity.y, rb.angular_velocity.z);
            t.rotation_quat += (0.5f * fixed_dt) * (omega_quat * t.rotation_quat);
            t.rotation_quat = glm::normalize(t.rotation_quat);
        });
}

// Simple sweep-and-prune broad phase.
void System::Collision(Scene& scene, float) {
  auto& colliders = scene.pool<ColliderComponent>();
  auto& transforms = scene.pool<TransformComponent>();
  auto& rigid_bodies = scene.pool<RigidBodyComponent>();
  auto& dense = colliders.dense_entities;

  const float default_restitution = 0.6f;

  // Detect added/removed colliders that invalidate the sweep array.
  std::size_t sweepable = 0;
  for (entity_t entity : dense) {
    if (!transforms.contains(entity)) continue;
    if (HasPlane(colliders.get(entity))) continue;
    ++sweepable;
  }
  bool rebuild = sweepable != sweep_entries_.size();
  if (!rebuild) {
    for (auto& entry : sweep_entries_) {
      if (!transforms.contains(entry.entity) || !colliders.contains(entry.entity)) {
        rebuild = true;
        break;
      }
      TransformComponent& t = transforms.get(entry.entity);
      entry.aabb = ComputeAABB(t.position, t.rotation_quat,
                               colliders.get(entry.entity));
    }
  }

  if (rebuild) {
    BuildAABBs(scene);
  } else {
    SortSweepEntries();
  }

  // Resolve a pair's contacts sequentially: each impulse updates the velocities
  // the next contact sees, so the points share the response (vn >= 0 skips ones
  // already separated) without per-contact scaling.
  auto resolve = [&](entity_t ea, entity_t eb, const ContactManifold& manifold) {
    RigidBodyComponent* ra = rigid_bodies.try_get(ea);
    RigidBodyComponent* rb = rigid_bodies.try_get(eb);
    const float wa = ra ? ra->inverse_mass : 0.0f;
    const float wb = rb ? rb->inverse_mass : 0.0f;
    const float total = wa + wb;
    float total_impulse = 0.0f;
    if (total == 0.0f) return total_impulse;

    TransformComponent& ta = transforms.get(ea);
    TransformComponent& tb = transforms.get(eb);

    auto apply_inv_inertia = [](const RigidBodyComponent* body, const glm::quat& q,
          const glm::vec3& v) {
      return q * (body->inverse_inertia * (glm::conjugate(q) * v));
      };

    // Corrections applied so far: each contact only fixes penetration not
    // already resolved along its normal, so a corner's floor and wall both
    // push out while same-normal contacts don't double-correct.
    glm::vec3 shift_a(0.0f), shift_b(0.0f);

    for (const Contact& c : manifold) {
      const float pen = c.penetration - glm::dot(shift_b - shift_a, c.normal);
      if (pen > 0.0f) {
        const glm::vec3 da = -c.normal * (pen * (wa / total));
        const glm::vec3 db =  c.normal * (pen * (wb / total));
        ta.position += da;
        tb.position += db;
        shift_a += da;
        shift_b += db;
      }

      const glm::vec3 lever_a = c.point - ta.position;
      const glm::vec3 lever_b = c.point - tb.position;

      const glm::vec3 va = ra ? ra->linear_velocity + glm::cross(ra->angular_velocity, lever_a)
          : glm::vec3(0.0f);
      const glm::vec3 vb = rb ? rb->linear_velocity + glm::cross(rb->angular_velocity, lever_b)
          : glm::vec3(0.0f);
      const float vn = glm::dot(vb - va, c.normal);
      if (vn >= 0.0f) continue;  // already separating at this contact point
      float denom = total;
      if (ra) denom += glm::dot(c.normal, glm::cross(
          apply_inv_inertia(ra, ta.rotation_quat, glm::cross(lever_a, c.normal)), lever_a));
      if (rb) denom += glm::dot(c.normal, glm::cross(
          apply_inv_inertia(rb, tb.rotation_quat, glm::cross(lever_b, c.normal)), lever_b));

      const float ra_rest = ra ? ra->restitution : default_restitution;
      const float rb_rest = rb ? rb->restitution : default_restitution;
      const float jimp = -(1.0f + 0.5f * (ra_rest + rb_rest)) * vn / denom;
      total_impulse += jimp;
      const glm::vec3 impulse = c.normal * jimp;
      if (ra) {
        ra->linear_velocity  -= impulse * wa;
        ra->angular_velocity -= apply_inv_inertia(ra, ta.rotation_quat, glm::cross(lever_a, impulse));
      }
      if (rb) {
        rb->linear_velocity  += impulse * wb;
        rb->angular_velocity += apply_inv_inertia(rb, tb.rotation_quat, glm::cross(lever_b, impulse));
      }
    }
    return total_impulse;
  };

  // Broad-phase sweep: narrow-test only AABB-overlapping pairs.
  ContactManifold manifold;
  for (std::size_t i = 0; i < sweep_entries_.size(); ++i) {
    const SweepEntry& a = sweep_entries_[i];
    for (std::size_t j = i + 1; j < sweep_entries_.size(); ++j) {
      const SweepEntry& b = sweep_entries_[j];
      if (b.aabb.min.x > a.aabb.max.x) break;
      if (a.aabb.min.y > b.aabb.max.y || a.aabb.max.y < b.aabb.min.y) continue;
      if (a.aabb.min.z > b.aabb.max.z || a.aabb.max.z < b.aabb.min.z) continue;
      TransformComponent& ta = transforms.get(a.entity);
      TransformComponent& tb = transforms.get(b.entity);
      manifold.Clear();
      Collide(ta.position, ta.rotation_quat, colliders.get(a.entity),
              tb.position, tb.rotation_quat, colliders.get(b.entity), manifold);
      if (manifold.count > 0) {
        const float j = resolve(a.entity, b.entity, manifold);
        const Contact& c = manifold.contacts[0];
        scene.GetEventBus().Publish(CollisionEvent{
        Entity(a.entity), Entity(b.entity), c.normal, c.point, c.penetration, j});
      }
    }
  }

  // Plane colliders are excluded from the sweep and handled separately
  for (entity_t plane_entity : dense) {
    ColliderComponent& plane_collider = colliders.get(plane_entity);
    if (!HasPlane(plane_collider)) continue;
    TransformComponent* tp = transforms.try_get(plane_entity);
    const glm::vec3 plane_pos = tp ? tp->position : glm::vec3(0.0f);
    const glm::quat plane_rot_quat = tp ? tp->rotation_quat : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    for (const SweepEntry& entry : sweep_entries_) {
      TransformComponent* te = transforms.try_get(entry.entity);
      manifold.Clear();
      Collide(te->position, te->rotation_quat, colliders.get(entry.entity),
              plane_pos, plane_rot_quat, plane_collider, manifold);
      if (manifold.count > 0) {
        const float j = resolve(entry.entity, plane_entity, manifold);
        const Contact& c = manifold.contacts[0];
        scene.GetEventBus().Publish(CollisionEvent{
        Entity(entry.entity), Entity(plane_entity), c.normal, c.point, c.penetration, j});
      }
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
  glm::mat4 view_proj = renderer.GetViewProjectionMatrix();
          auto row = [&](const glm::mat4& m, int i){
            return glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]);
          };
          glm::vec4 frustum_planes[6] = {
            row(view_proj,3) + row(view_proj,0),  // left
            row(view_proj,3) - row(view_proj,0),  // right
            row(view_proj,3) + row(view_proj,1),  // bottom
            row(view_proj,3) - row(view_proj,1),  // top
            row(view_proj,3) + row(view_proj,2),  // near
            row(view_proj,3) - row(view_proj,2),  // far
          };
          for (auto& p : frustum_planes) p /= glm::length(glm::vec3(p)); // normalize

  // filtering lambda which any part of the mesh's AABB box is in the frustum
  auto inside_frustum = [&](const glm::mat4& model, const glm::vec3& bmin,
                            const glm::vec3& bmax) -> bool {
    const glm::vec3 center = 0.5f * (bmin + bmax);
    const glm::vec3 half   = 0.5f * (bmax - bmin);
    const glm::vec3 world_center = glm::vec3(model * glm::vec4(center, 1.0f));
    const glm::mat3 abs_model(glm::abs(glm::vec3(model[0])),
                              glm::abs(glm::vec3(model[1])),
                              glm::abs(glm::vec3(model[2])));
    const glm::vec3 world_half = abs_model * half;
    for (const glm::vec4& plane : frustum_planes) {
      const float dist   = glm::dot(glm::vec3(plane), world_center) + plane.w;
      const float radius = glm::dot(glm::abs(glm::vec3(plane)), world_half);
      if (dist + radius < 0.0f) return false;  // fully behind this plane
    }
    return true;
  };

  auto& camera = scene.pool<CameraComponent>().get(0); // get the first camera component
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
          model *= glm::mat4_cast(transform.rotation_quat);
          model  = glm::scale(model, transform.scale);
  
          std::uint32_t tex = texture_pool.contains(entity)
              ? texture_pool.get(entity).texture.id : 0;
          std::uint64_t key = (std::uint64_t(mesh.mesh.id) << 32) | tex;

          // frustum culling
          glm::vec3 bmin, bmax;
          if (renderer.GetMeshBounds(mesh.mesh, bmin, bmax) &&
              !inside_frustum(model, bmin, bmax)) {
            return;
          }
          
          
          




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

// Full rebuild of the sweep array, used on startup and when colliders are
// added/removed.
void System::BuildAABBs(Scene& scene) {
  auto& colliders  = scene.pool<ColliderComponent>();
  auto& transforms = scene.pool<TransformComponent>();

  sweep_entries_.clear();
  for (entity_t entity : colliders.dense_entities) {
    if (!transforms.contains(entity)) continue;
    const ColliderComponent& collider = colliders.get(entity);
    if (HasPlane(collider)) continue;  // planes get their own pass
    TransformComponent& t = transforms.get(entity);
    sweep_entries_.push_back(
        {entity, ComputeAABB(t.position, t.rotation_quat, collider)});
  }

  std::sort(sweep_entries_.begin(), sweep_entries_.end(),
            [](const SweepEntry& a, const SweepEntry& b) {
              return a.aabb.min.x < b.aabb.min.x;
            });
}

// Insertion sort by min.x
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
