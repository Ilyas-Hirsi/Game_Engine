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
#include "../Camera.h"
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
  scene.view<TransformComponent, MovementComponent>().par_each(scene.GetTaskScheduler(),
    [&](uint32_t, TransformComponent& t, MovementComponent& m) {
      t.position += m.move_direction * m.speed * delta_time;
    }
  );
}

void System::FixedUpdate(Scene& scene, float fixed_dt) {
    // Manage Gravity
    auto& physics_settings = scene.GetPhysicsSettings();
    scene.view<TransformComponent, RigidBodyComponent>().par_each(scene.GetTaskScheduler(),
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

void System::Collision(Scene& scene, float delta_time) {
  (void)delta_time;

  auto& colliders = scene.pool<ColliderComponent>();
  auto& transforms = scene.pool<TransformComponent>();
  auto& rigid_bodies = scene.pool<RigidBodyComponent>();

  const float default_restitution = 0.6f;

  // Apply queued removals, then insertions, before any query.
  for (entity_t entity : scene.GetCollidersToDestroy()) {
    auto d = dynamic_proxies_.find(entity);
    if (d != dynamic_proxies_.end()) {
      dynamic_tree_.RemoveLeaf(d->second);
      dynamic_proxies_.erase(d);
      continue;
    }
    auto s = static_proxies_.find(entity);
    if (s != static_proxies_.end()) {
      static_tree_.RemoveLeaf(s->second);
      static_proxies_.erase(s);
    }
  }
  scene.GetCollidersToDestroy().clear();

  for (entity_t entity : scene.GetCollidersToCreate()) {
    if (!colliders.contains(entity) || !transforms.contains(entity)) continue;
    const ColliderComponent& collider = colliders.get(entity);
    if (HasPlane(collider)) continue;  // planes never enter the trees
    const TransformComponent& t = transforms.get(entity);
    const AABB box = ComputeAABB(t.position, t.rotation_quat, collider);
    if (collider.is_static) {
      static_proxies_[entity] = static_tree_.InsertLeaf(entity, box);
    } else {
      dynamic_proxies_[entity] = dynamic_tree_.InsertLeaf(entity, box);
    }
  }
  scene.GetCollidersToCreate().clear();

  // Refresh dynamic proxies to their post-integration positions.
  for (auto& [entity, proxy] : dynamic_proxies_) {
    if (!transforms.contains(entity) || !colliders.contains(entity)) continue;
    const TransformComponent& t = transforms.get(entity);
    proxy = dynamic_tree_.UpdateLeaf(
        proxy, ComputeAABB(t.position, t.rotation_quat, colliders.get(entity)));
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
    const Contact& c = manifold.contacts[0];
    scene.GetEventBus().Publish(CollisionEvent{
    Entity(ea), Entity(eb), c.normal, c.point, c.penetration, total_impulse});
    return total_impulse;
  };

  std::vector<entity_t> dynamic_entities;
  dynamic_entities.reserve(dynamic_proxies_.size());
  for (const auto& [entity, proxy] : dynamic_proxies_) {
    (void)proxy;
    dynamic_entities.push_back(entity);
  }

  std::vector<entity_t> plane_entities;
  for (entity_t plane_entity : colliders.dense_entities) {
    if (HasPlane(colliders.get(plane_entity))) plane_entities.push_back(plane_entity);
  }

  std::vector<std::vector<entity_t>> candidates(dynamic_entities.size());
  scene.GetTaskScheduler().parallel_for(0, dynamic_entities.size(), [&](std::size_t i) {
    const entity_t entity = dynamic_entities[i];
    if (!transforms.contains(entity) || !colliders.contains(entity)) return;
    const TransformComponent& t = transforms.get(entity);
    const AABB box = ComputeAABB(t.position, t.rotation_quat, colliders.get(entity));

    dynamic_tree_.Query(box, [&](entity_t other) {
      if (other > entity) candidates[i].push_back(other);
    });
    static_tree_.Query(box, [&](entity_t other) {
      candidates[i].push_back(other);
    });
  });

  struct PairContact { entity_t a; entity_t b; ContactManifold manifold; };
  std::vector<std::vector<PairContact>> pair_contacts(dynamic_entities.size());
  scene.GetTaskScheduler().parallel_for(0, dynamic_entities.size(), [&](std::size_t i) {
    const entity_t entity = dynamic_entities[i];
    TransformComponent* te = transforms.try_get(entity);
    if (!te || !colliders.contains(entity)) return;
    const ColliderComponent& ce = colliders.get(entity);

    for (entity_t other : candidates[i]) {
      TransformComponent* to = transforms.try_get(other);
      if (!to || !colliders.contains(other)) continue;
      ContactManifold manifold;
      Collide(te->position, te->rotation_quat, ce,
              to->position, to->rotation_quat, colliders.get(other), manifold);
      if (manifold.count > 0) pair_contacts[i].push_back({entity, other, manifold});
    }

    for (entity_t plane_entity : plane_entities) {
      TransformComponent* tp = transforms.try_get(plane_entity);
      const glm::vec3 plane_pos = tp ? tp->position : glm::vec3(0.0f);
      const glm::quat plane_rot_quat = tp ? tp->rotation_quat : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
      ContactManifold manifold;
      Collide(te->position, te->rotation_quat, ce,
              plane_pos, plane_rot_quat, colliders.get(plane_entity), manifold);
      if (manifold.count > 0) pair_contacts[i].push_back({entity, plane_entity, manifold});
    }
  });

  for (std::size_t i = 0; i < pair_contacts.size(); ++i) {
    for (const PairContact& pc : pair_contacts[i]) {
      resolve(pc.a, pc.b, pc.manifold);
    }
  }
}

void System::UpdateCamera(Scene& scene, Renderer& renderer) {
  // No active camera leaves the previous frame's matrices in place.
  if (auto matrices = scene.ActiveCameraMatrices(renderer.GetAspectRatio())) {
    renderer.SetCamera(matrices->view, matrices->projection);
  }
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

}  // namespace engine
