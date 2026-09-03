#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>
#include <variant>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Components/ColliderComponent.h"

namespace engine {

// Narrow-phase result. `normal` points A->B (unit when `hit`); `penetration`
// is the overlap depth. Resolution moves A along -normal, B along +normal.
struct Contact {
  bool hit = false;
  glm::vec3 normal = glm::vec3(0.0f);
  float penetration = 0.0f;
  glm::vec3 point = glm::vec3(0.0f);
};


struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
  glm::vec3 inv_direction;
  float t_min;
  float t_max;

};


inline Contact Flip(Contact c) {
  c.normal = -c.normal;
  return c;
}

// Keeps the deepest contacts in a heap ordered by penetration depth.
struct ContactManifold {
  static constexpr int kMaxContacts = 8;
  Contact contacts[kMaxContacts];
  int     count = 0;

  
  static bool ShallowerAtRoot(const Contact& a, const Contact& b) {
    return a.penetration > b.penetration;
  }

  void Add(const Contact& c) {
    if (!c.hit) return;
    if (count < kMaxContacts) {
      contacts[count++] = c;
      std::push_heap(contacts, contacts + count, ShallowerAtRoot);
    } else if (c.penetration > contacts[0].penetration) {
      std::pop_heap(contacts, contacts + kMaxContacts, ShallowerAtRoot);
      contacts[kMaxContacts - 1] = c;
      std::push_heap(contacts, contacts + kMaxContacts, ShallowerAtRoot);
    }
  }

  void Clear() { count = 0; }

  const Contact* begin() const { return contacts; }
  const Contact* end()   const { return contacts + count; }
};

// --- Narrow phase ---------------------------------------------------------
// `ca` / `cb` are world positions from TransformComponent. Planes are infinite
// half-spaces defined by normal + offset, so they ignore the transform.

inline Contact SphereSphere(const glm::vec3& ca, const Sphere& a,
                            const glm::vec3& cb, const Sphere& b) {
  const glm::vec3 d = cb - ca;  // A -> B
  const float dist2 = glm::dot(d, d);
  const float r = a.radius + b.radius;
  if (dist2 >= r * r) return {};

  const float dist = std::sqrt(dist2);
  const glm::vec3 n = dist > 1e-6f ? d / dist : glm::vec3(1.0f, 0.0f, 0.0f);
  const glm::vec3 point = ca + n * a.radius;
  return {true, n, r - dist, point};
}

inline Contact SpherePlane(const glm::vec3& ca, const Sphere& a, const Plane& p) {
  const float signed_dist = glm::dot(p.normal, ca) - p.offset;
  const float pen = a.radius - signed_dist;
  if (pen <= 0.0f) return {};
  const glm::vec3 point = ca - p.normal * a.radius;
  return {true, -p.normal, pen, point};
}

inline Contact SphereBox(const glm::vec3& ca, const Sphere& a,
                         const glm::vec3& cb, const glm::quat& rb, const Box& b) {
      const glm::vec3 local = glm::conjugate(rb) * (ca - cb);  // sphere center in box space
      const glm::vec3 closest = glm::clamp(local, -b.half_extents, b.half_extents);
      const glm::vec3 delta = local - closest;
      const float dist2 = glm::dot(delta, delta);
      if (dist2 >= a.radius * a.radius) return {};
      const float dist = std::sqrt(dist2);
      const glm::vec3 n_local = dist > 1e-6f ? -delta / dist : glm::vec3(0.0f, 1.0f, 0.0f);
      const glm::vec3 point = cb + rb * closest;
      return {true, rb * n_local, a.radius - dist, point};  // rotate normal back to world
}


inline Contact BoxPlane(const glm::vec3& ca, const glm::quat& ra, const Box& a, const Plane& p) {
  const glm::mat3 r = glm::mat3_cast(ra);

  const glm::vec3 ex = r[0] * a.half_extents.x;
  const glm::vec3 ey = r[1] * a.half_extents.y;
  const glm::vec3 ez = r[2] * a.half_extents.z;

  // signed reach of each scaled axis along the plane normal; a vertex's
  // distance to the plane is signed_dist +/- s.x +/- s.y +/- s.z
  const glm::vec3 s(glm::dot(p.normal, ex),
                    glm::dot(p.normal, ey),
                    glm::dot(p.normal, ez));
  const float rad = std::abs(s.x) + std::abs(s.y) + std::abs(s.z);
  const float signed_dist = glm::dot(p.normal, ca) - p.offset;
  const float pen = rad - signed_dist;
  if (pen <= 0.0f) return {};

  // average all penetrating vertices: stable for face/edge resting contacts
  glm::vec3 sum(0.0f);
  int count = 0;
  for (int i = 0; i < 8; ++i) {
    const float sx = (i & 1) ? 1.0f : -1.0f;
    const float sy = (i & 2) ? 1.0f : -1.0f;
    const float sz = (i & 4) ? 1.0f : -1.0f;
    if (signed_dist + sx * s.x + sy * s.y + sz * s.z >= 0.0f) continue;
    sum += ca + sx * ex + sy * ey + sz * ez;
    ++count;
  }

  Contact c{true, -p.normal, pen};
  c.point = sum / float(count);
  return c;
}

inline Contact BoxBox(const glm::vec3& ca, const glm::quat& qa, const Box& a,
                      const glm::vec3& cb, const glm::quat& qb, const Box& b) {
  const glm::mat3 ra = glm::mat3_cast(qa);
  const glm::mat3 rb = glm::mat3_cast(qb);
  const glm::vec3 d = cb - ca;  // A -> B
  float best_pen = std::numeric_limits<float>::infinity();
  glm::vec3 best_axis(0.0f);
  const auto test_axis = [&](glm::vec3 axis) -> bool {
    const float len2 = glm::dot(axis, axis);
    if (len2 < 1e-8f) return true;  // near-parallel edges: cross product degenerate, skip
    axis /= std::sqrt(len2);
    const float proj_a = a.half_extents.x * std::abs(glm::dot(axis, ra[0]))
                        + a.half_extents.y * std::abs(glm::dot(axis, ra[1]))
                        + a.half_extents.z * std::abs(glm::dot(axis, ra[2]));
    const float proj_b = b.half_extents.x * std::abs(glm::dot(axis, rb[0]))
                        + b.half_extents.y * std::abs(glm::dot(axis, rb[1]))
                        + b.half_extents.z * std::abs(glm::dot(axis, rb[2]));
    const float dist = glm::dot(axis, d);
    const float pen = proj_a + proj_b - std::abs(dist);
    if (pen <= 0.0f) return false;  // separating axis found -> no collision
    if (pen < best_pen) {
      best_pen = pen;
      best_axis = dist >= 0.0f ? axis : -axis;  // keep normal pointing A -> B
    }
    return true;
  };
  for (int i = 0; i < 3; ++i) if (!test_axis(ra[i])) return {};
  for (int i = 0; i < 3; ++i) if (!test_axis(rb[i])) return {};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      if (!test_axis(glm::cross(ra[i], rb[j]))) return {};
      glm::vec3 pa = ca, pb = cb;
      for (int i = 0; i < 3; ++i) {
        pa += ra[i] * (a.half_extents[i] * (glm::dot(ra[i], best_axis) >= 0.0f ? 1.0f : -1.0f));
        pb -= rb[i] * (b.half_extents[i] * (glm::dot(rb[i], best_axis) >= 0.0f ? 1.0f : -1.0f));
      }
      return {true, best_axis, best_pen, 0.5f * (pa + pb)};
}

inline Contact CapsuleSphere(const glm::vec3& ca, const glm::quat& ra, const Capsule& a,
                             const glm::vec3& cb, const Sphere& b) {

      const glm::vec3 axis = ra * glm::vec3(0.0f, 1.0f, 0.0f);
      const glm::vec3 d = cb - ca;
      float t = glm::dot(d, axis);
      t = glm::clamp(t, -a.height * 0.5f, a.height * 0.5f);
      const glm::vec3 closest = ca + axis * t;
      return SphereSphere(closest, Sphere{a.radius}, cb, b);
}

inline Contact CapsulePlane(const glm::vec3& ca, const glm::quat& ra, const Capsule& a, const Plane& p) {
  const glm::vec3 axis = ra * glm::vec3(0.0f, 1.0f, 0.0f);
  const float half = a.height * 0.5f;
  const glm::vec3 e0 = ca + axis * half;
  const glm::vec3 e1 = ca - axis * half;
  const float d0 = glm::dot(p.normal, e0) - p.offset;
  const float d1 = glm::dot(p.normal, e1) - p.offset;
  // deepest endpoint acts as the sphere; lying flat, both ends are equally
  // deep and the midpoint gives a stable line-contact stand-in
  glm::vec3 deepest = d0 < d1 ? e0 : e1;
  if (std::abs(d0 - d1) < 1e-4f) deepest = ca;
  return SpherePlane(deepest, Sphere{a.radius}, p);
}

// Closest points between segments p1->q1 and p2->q2 (Ericson, RTCD 5.1.9).
// Assumes both segments have positive length, which capsule cores always do.
inline void ClosestPointsSegmentSegment(const glm::vec3& p1, const glm::vec3& q1,
                                        const glm::vec3& p2, const glm::vec3& q2,
                                        glm::vec3& c1, glm::vec3& c2) {
  const glm::vec3 d1 = q1 - p1;
  const glm::vec3 d2 = q2 - p2;
  const glm::vec3 r = p1 - p2;
  const float aa = glm::dot(d1, d1);
  const float e = glm::dot(d2, d2);
  const float b = glm::dot(d1, d2);
  const float c = glm::dot(d1, r);
  const float f = glm::dot(d2, r);
  const float denom = aa * e - b * b;  // ~0 when the segments are parallel
  float s = denom > 1e-8f ? glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
  float t = (b * s + f) / e;
  if (t < 0.0f) {
    t = 0.0f;
    s = glm::clamp(-c / aa, 0.0f, 1.0f);
  } else if (t > 1.0f) {
    t = 1.0f;
    s = glm::clamp((b - c) / aa, 0.0f, 1.0f);
  }
  c1 = p1 + d1 * s;
  c2 = p2 + d2 * t;
}

inline Contact CapsuleCapsule(const glm::vec3& ca, const glm::quat& qa, const Capsule& a,
                              const glm::vec3& cb, const glm::quat& qb, const Capsule& b) {
  const glm::vec3 axis_a = qa * glm::vec3(0.0f, 1.0f, 0.0f);
  const glm::vec3 axis_b = qb * glm::vec3(0.0f, 1.0f, 0.0f);
  const float ha = a.height * 0.5f;
  const float hb = b.height * 0.5f;
  glm::vec3 closest_a, closest_b;
  ClosestPointsSegmentSegment(ca - axis_a * ha, ca + axis_a * ha,
                              cb - axis_b * hb, cb + axis_b * hb,
                              closest_a, closest_b);
  return SphereSphere(closest_a, Sphere{a.radius}, closest_b, Sphere{b.radius});
}

inline Contact CapsuleBox(const glm::vec3& ca, const glm::quat& qa, const Capsule& a,
                          const glm::vec3& cb, const glm::quat& qb, const Box& b) {
  // capsule core segment in box-local space
  const glm::vec3 axis = qa * glm::vec3(0.0f, 1.0f, 0.0f);
  const float half = a.height * 0.5f;
  const glm::quat inv = glm::conjugate(qb);
  const glm::vec3 p0 = inv * (ca - axis * half - cb);
  const glm::vec3 p1 = inv * (ca + axis * half - cb);

  // alternate clamp-to-box / project-to-segment: converges on the closest
  // segment point for the shallow contacts we resolve
  const glm::vec3 seg_dir = p1 - p0;
  const float len2 = glm::dot(seg_dir, seg_dir);
  glm::vec3 seg_point = 0.5f * (p0 + p1);
  for (int iter = 0; iter < 4; ++iter) {
    const glm::vec3 box_point = glm::clamp(seg_point, -b.half_extents, b.half_extents);
    const float t = len2 > 1e-8f
        ? glm::clamp(glm::dot(box_point - p0, seg_dir) / len2, 0.0f, 1.0f)
        : 0.0f;
    seg_point = p0 + seg_dir * t;
  }

  const glm::vec3 world_point = cb + qb * seg_point;
  return SphereBox(world_point, Sphere{a.radius}, cb, qb, b);
}

inline void Collide(const glm::vec3& pa, const glm::quat& ra, const ColliderComponent& a,
                       const glm::vec3& pb, const glm::quat& rb, const ColliderComponent& b,
                       ContactManifold& out) {

  for (const ChildShape& child_a : a.child_shapes) {
    for (const ChildShape& child_b : b.child_shapes) {
      glm::vec3 child_a_pos = pa + ra * child_a.local_position;
      glm::quat child_a_rot = ra * child_a.local_rotation;
      glm::vec3 child_b_pos = pb + rb * child_b.local_position;
      glm::quat child_b_rot = rb * child_b.local_rotation;
      std::visit(
        [&](auto const& sa, auto const& sb) -> void {
          using A = std::decay_t<decltype(sa)>;
          using B = std::decay_t<decltype(sb)>;
          if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Sphere>)
            out.Add(SphereSphere(child_a_pos, sa, child_b_pos, sb));
          else if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Plane>)
            out.Add(SpherePlane(child_a_pos, sa, sb));
          else if constexpr (std::is_same_v<A, Plane> && std::is_same_v<B, Sphere>)
            out.Add(Flip(SpherePlane(child_b_pos, sb, sa)));
          else if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Box>)
            out.Add(SphereBox(child_a_pos, sa, child_b_pos, child_b_rot, sb));
          else if constexpr (std::is_same_v<A, Box> && std::is_same_v<B, Sphere>)
            out.Add(Flip(SphereBox(child_b_pos, sb, child_a_pos, child_a_rot, sa)));
          else if constexpr (std::is_same_v<A, Box> && std::is_same_v<B, Plane>)
            out.Add(BoxPlane(child_a_pos, child_a_rot, sa, sb));
          else if constexpr (std::is_same_v<A, Plane> && std::is_same_v<B, Box>)
            out.Add(Flip(BoxPlane(child_b_pos, child_b_rot, sb, sa)));
          else if constexpr (std::is_same_v<A, Box> && std::is_same_v<B, Box>)
            out.Add(BoxBox(child_a_pos, child_a_rot, sa, child_b_pos, child_b_rot, sb));
          else if constexpr (std::is_same_v<A, Capsule> && std::is_same_v<B, Sphere>)
            out.Add(CapsuleSphere(child_a_pos, child_a_rot, sa, child_b_pos, sb));
          else if constexpr (std::is_same_v<A, Sphere> && std::is_same_v<B, Capsule>)
            out.Add(Flip(CapsuleSphere(child_b_pos, child_b_rot, sb, child_a_pos, sa)));
          else if constexpr (std::is_same_v<A, Capsule> && std::is_same_v<B, Plane>)
            out.Add(CapsulePlane(child_a_pos, child_a_rot, sa, sb));
          else if constexpr (std::is_same_v<A, Plane> && std::is_same_v<B, Capsule>)
            out.Add(Flip(CapsulePlane(child_b_pos, child_b_rot, sb, sa)));
          else if constexpr (std::is_same_v<A, Capsule> && std::is_same_v<B, Box>)
            out.Add(CapsuleBox(child_a_pos, child_a_rot, sa, child_b_pos, child_b_rot, sb));
          else if constexpr (std::is_same_v<A, Box> && std::is_same_v<B, Capsule>)
            out.Add(Flip(CapsuleBox(child_b_pos, child_b_rot, sb, child_a_pos, child_a_rot, sa)));
          else if constexpr (std::is_same_v<A, Capsule> && std::is_same_v<B, Capsule>)
            out.Add(CapsuleCapsule(child_a_pos, child_a_rot, sa, child_b_pos, child_b_rot, sb));
            // plane-plane collision ignored
        },
        child_a.shape, child_b.shape);

    }

  }
}

inline AABB ComputeAABB(const glm::vec3& pos, const glm::quat& rot, const ColliderComponent& col){
  AABB aabb;
  float infinity = std::numeric_limits<float>::infinity();
  aabb.min = {infinity, infinity, infinity};
  aabb.max = {-infinity, -infinity, -infinity};
  for (const ChildShape& child : col.child_shapes) {
    glm::vec3 child_pos = pos + rot * child.local_position;
    glm::quat child_rot = rot * child.local_rotation;
std::visit(
  [&](auto const& col) -> void {
    using T = std::decay_t<decltype(col)>;
    if constexpr (std::is_same_v<T, Sphere>) {
      aabb.min = glm::min(aabb.min, child_pos - glm::vec3(col.radius));
      aabb.max = glm::max(aabb.max, child_pos + glm::vec3(col.radius));
    } else if constexpr (std::is_same_v<T, Box>) {
      const glm::mat3 r = glm::mat3_cast(child_rot);
      const glm::mat3 abs_r(glm::abs(r[0]), glm::abs(r[1]), glm::abs(r[2]));
      const glm::vec3 world_half = abs_r * col.half_extents;
      aabb.min = glm::min(aabb.min, child_pos - world_half);
      aabb.max = glm::max(aabb.max, child_pos + world_half);
    } else if constexpr (std::is_same_v<T, Plane>) {
      // Planes are infinite and handled in a separate pass, so they add
      // nothing here; a plane-only collider keeps the inverted-infinity box.
    }
    else if constexpr (std::is_same_v<T, Capsule>) {
      const float half_height = col.height * 0.5f;
      const glm::vec3 axis = child_rot * glm::vec3(0.0f, half_height, 0.0f);
      const glm::vec3 world_half = glm::abs(axis) + glm::vec3(col.radius);
      aabb.min = glm::min(aabb.min, child_pos - world_half);
      aabb.max = glm::max(aabb.max, child_pos + world_half);
    }
  },
  child.shape);

  }
return aabb;
}

// Planes are infinite half-spaces, so plane colliders are excluded from the
// sweep and handled in their own pass (see System::Collision).
inline bool HasPlane(const ColliderComponent& col) {
  for (const ChildShape& child : col.child_shapes)
    if (std::holds_alternative<Plane>(child.shape)) return true;
  return false;
}

}  // namespace engine
