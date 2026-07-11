#pragma once

#include <cmath>
#include <limits>
#include <type_traits>
#include <variant>
#include <glm/glm.hpp>

#include "Components/ColliderComponent.h"

namespace engine {

// Result of a narrow-phase test. `normal` points from A toward B and is unit
// length when `hit` is true; `penetration` is how deep the two shapes overlap.
// Resolution moves A along -normal and B along +normal.
struct Contact {
  bool      hit         = false;
  glm::vec3 normal      = glm::vec3(0.0f);
  float     penetration = 0.0f;
};
struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

inline Contact Flip(Contact c) {
  c.normal = -c.normal;
  return c;
}

// --- Narrow phase ---------------------------------------------------------
// World placement comes from the entity's TransformComponent.position, passed
// in here as `ca` / `cb`. Planes are infinite half-spaces defined absolutely
// by their normal + offset, so they ignore the transform position.

inline Contact SphereSphere(const glm::vec3& ca, const Sphere& a,
                            const glm::vec3& cb, const Sphere& b) {
  const glm::vec3 d = cb - ca;  // A -> B



  const float dist2 = glm::dot(d, d);
  const float r = a.radius + b.radius;
  if (dist2 >= r * r) return {};

  const float dist = std::sqrt(dist2);
  const glm::vec3 n = dist > 1e-6f ? d / dist : glm::vec3(1.0f, 0.0f, 0.0f);
  return {true, n, r - dist};
}

inline Contact SpherePlane(const glm::vec3& ca, const Sphere& a, const Plane& p) {
  const float signed_dist = glm::dot(p.normal, ca) - p.offset;
  const float pen = a.radius - signed_dist;
  if (pen <= 0.0f) return {};
  return {true, -p.normal, pen};
}

inline Contact SphereBox(const glm::vec3& ca, const Sphere& a,
                         const glm::vec3& cb, const Box& b) {
  const glm::vec3 local = ca - cb;  // sphere center relative to box center
  const glm::vec3 closest = glm::clamp(local, -b.half_extents, b.half_extents);
  const glm::vec3 delta = local - closest;  // box surface -> sphere center
  const float dist2 = glm::dot(delta, delta);
  if (dist2 >= a.radius * a.radius) return {};

  const float dist = std::sqrt(dist2);
  // A->B = sphere -> box = -delta (delta points box -> sphere).
  const glm::vec3 n = dist > 1e-6f ? -delta / dist : glm::vec3(0.0f, 1.0f, 0.0f);
  return {true, n, a.radius - dist};
}

inline Contact Collide(const glm::vec3& pa, const ColliderComponent& a,
                       const glm::vec3& pb, const ColliderComponent& b) {
  return std::visit(
      [&](auto const& sa, auto const& sb) -> Contact {
        using A = std::decay_t<decltype(sa)>;
        using B = std::decay_t<decltype(sb)>;
        if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Sphere>)
          return SphereSphere(pa, sa, pb, sb);
        else if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Plane>)
          return SpherePlane(pa, sa, sb);
        else if constexpr (std::is_same_v<A, Plane> && std::is_same_v<B, Sphere>)
          return Flip(SpherePlane(pb, sb, sa));
        else if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Box>)
          return SphereBox(pa, sa, pb, sb);
        else if constexpr (std::is_same_v<A, Box> && std::is_same_v<B, Sphere>)
          return Flip(SphereBox(pb, sb, pa, sa));
        else
          return {};  // box-box, plane-plane, plane-box: not handled yet
      },
      a.shape, b.shape);
}

// Planes are unbounded, so they never enter the sweep array; callers filter
// them out first. If one slips through it gets the "empty box" sentinel
// (min > max), which can never overlap anything.
inline AABB ComputeAABB(const glm::vec3& pos, const ColliderComponent& col) {
  AABB aabb;
  const float infinity = std::numeric_limits<float>::infinity();
  std::visit(
      [&](auto const& shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, Sphere>) {
          aabb.min = pos - glm::vec3(shape.radius);
          aabb.max = pos + glm::vec3(shape.radius);
        } else if constexpr (std::is_same_v<T, Box>) {
          aabb.min = pos - shape.half_extents;
          aabb.max = pos + shape.half_extents;
        } else if constexpr (std::is_same_v<T, Plane>) {
          aabb.min = glm::vec3(infinity);
          aabb.max = glm::vec3(-infinity);
        }
      },
      col.shape);
  return aabb;
}

}  // namespace engine
