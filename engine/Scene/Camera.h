#pragma once
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Collision.h"
#include "Components/CameraComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/TransformComponent.h"

namespace engine {

struct CameraMatrices {
  glm::mat4 view;
  glm::mat4 projection;
};

// Aspect comes from the viewport
inline CameraMatrices ComputeCamera(const CameraComponent& camera,
                                    const TransformComponent& transform,
                                    const MovementComponent& movement,
                                    float aspect) {
  CameraMatrices out;
  out.projection = glm::perspective(glm::radians(camera.fov), aspect,
                                    camera.near_plane, camera.far_plane);
  out.view = glm::lookAt(transform.position,
                         transform.position + movement.facing,
                         glm::vec3(0.0f, 1.0f, 0.0f));
  return out;
}

inline Ray MakeRay(const glm::vec3& origin, const glm::vec3& direction,
                   float t_max = std::numeric_limits<float>::max()) {
  Ray ray;
  ray.origin = origin;
  ray.direction = glm::normalize(direction);
  ray.inv_direction = 1.0f / ray.direction;
  ray.t_min = 0.0f;
  ray.t_max = t_max;
  return ray;
}

inline Ray ScreenPointToRay(const CameraMatrices& camera, int x, int y,
                            int viewport_width, int viewport_height) {
  // Window coordinates start top left, NDC starts bottom left.
  const float ndc_x = (2.0f * x) / viewport_width - 1.0f;
  const float ndc_y = 1.0f - (2.0f * y) / viewport_height;

  const glm::mat4 inverse_vp = glm::inverse(camera.projection * camera.view);
  glm::vec4 near_point = inverse_vp * glm::vec4(ndc_x, ndc_y, -1.0f, 1.0f);
  glm::vec4 far_point  = inverse_vp * glm::vec4(ndc_x, ndc_y,  1.0f, 1.0f);
  near_point /= near_point.w;
  far_point  /= far_point.w;

  return MakeRay(glm::vec3(near_point), glm::vec3(far_point - near_point));
}

}  // namespace engine