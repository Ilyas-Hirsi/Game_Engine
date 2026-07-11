#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/ColliderComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"
#include "core/Log.h"
#include "core/Timer.h"
#include "platform/MeshFactory.h"
#include "platform/Renderer.h"
#include "platform/Window.h"
#include <iostream>
#include <random>


int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  // Arena is a +/-kHalf box. The floor sits at y = -kHalf; four invisible
  // wall planes face inward so nothing escapes sideways.
  const float kHalf        = 100.0f;
  const float kSphereScale = 4.0f;                 // mesh radius 0.5 -> visual 2.0
  const float kRadius      = 0.5f * kSphereScale;  // collider radius matches
  const int   kSphereCount = 1500;

  engine::Window window("Collision Demo", 1280, 720);

  engine::Renderer renderer;
  if (!renderer.Initialize(window)) {
    engine::LogError("Failed to initialize renderer.");
    return 1;
  }

  engine::Scene scene;

  // --- Camera ---------------------------------------------------------------
  engine::Entity camera = scene.CreateEntity();
  engine::TransformComponent camera_transform{};
  engine::MovementComponent camera_movement{};
  camera_transform.position = glm::vec3(0.0f, 20.0f, 130.0f);
  camera_movement.facing = glm::normalize(glm::vec3(0.0f, -0.25f, -1.0f));
  camera_movement.speed = 40.0f;
  scene.emplace<engine::TransformComponent>(camera, camera_transform);

  engine::CameraComponent camera_component{};
  camera_component.fov = 60.0f;
  camera_component.near_plane = 0.1f;
  camera_component.far_plane = 1000.0f;
  scene.emplace<engine::CameraComponent>(camera, camera_component);
  engine::RigidBodyComponent camera_rigid_body{};
  camera_rigid_body.inverse_mass = 0.0f;
  scene.emplace<engine::RigidBodyComponent>(camera, camera_rigid_body);
  scene.emplace<engine::MovementComponent>(camera, camera_movement);
  scene.emplace<engine::InputComponent>(camera, engine::InputComponent{});

  // --- Floor (visible plane mesh + static plane collider) -------------------
  const engine::MeshHandle plane_mesh =
      renderer.CreateMesh(engine::MeshFactory::Plane());

  engine::Entity floor = scene.CreateEntity();
  engine::TransformComponent floor_transform{};
  floor_transform.position = glm::vec3(0.0f, -kHalf, 0.0f);
  floor_transform.scale = glm::vec3(2.0f * kHalf, 1.0f, 2.0f * kHalf);
  scene.emplace<engine::TransformComponent>(floor, floor_transform);

  engine::MeshComponent floor_mesh{};
  floor_mesh.mesh = plane_mesh;
  floor_mesh.entity_id = floor.GetId();
  scene.emplace<engine::MeshComponent>(floor, floor_mesh);

  engine::ColliderComponent floor_collider{};
  floor_collider.shape = engine::Plane{glm::vec3(0.0f, 1.0f, 0.0f), -kHalf};
  floor_collider.is_static = true;
  scene.emplace<engine::ColliderComponent>(floor, floor_collider);

  // --- Containment walls (invisible static plane colliders) -----------------
  auto add_wall = [&](glm::vec3 normal) {
    engine::Entity wall = scene.CreateEntity();
    scene.emplace<engine::TransformComponent>(wall, engine::TransformComponent{});
    engine::ColliderComponent collider{};
    collider.shape = engine::Plane{normal, -kHalf};
    collider.is_static = true;
    scene.emplace<engine::ColliderComponent>(wall, collider);
  };
  add_wall(glm::vec3( 1.0f, 0.0f,  0.0f));  // left  (x >= -kHalf)
  add_wall(glm::vec3(-1.0f, 0.0f,  0.0f));  // right (x <=  kHalf)
  add_wall(glm::vec3( 0.0f, 0.0f,  1.0f));  // back  (z >= -kHalf)
  add_wall(glm::vec3( 0.0f, 0.0f, -1.0f));  // front (z <=  kHalf)

  // --- Falling spheres ------------------------------------------------------
  const engine::MeshHandle sphere_mesh =
      renderer.CreateMesh(engine::MeshFactory::Sphere());

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<float> spread(-kHalf + kRadius, kHalf - kRadius);
  std::uniform_real_distribution<float> height(10.0f, 80.0f);

  for (int i = 0; i < kSphereCount; ++i) {
    engine::Entity sphere = scene.CreateEntity();

    engine::TransformComponent transform{};
    transform.position = glm::vec3(spread(rng), height(rng), spread(rng));
    transform.scale = glm::vec3(kSphereScale);
    scene.emplace<engine::TransformComponent>(sphere, transform);

    engine::MeshComponent mesh{};
    mesh.mesh = sphere_mesh;
    mesh.entity_id = sphere.GetId();
    scene.emplace<engine::MeshComponent>(sphere, mesh);

    engine::ColliderComponent collider{};
    collider.shape = engine::Sphere{kRadius};
    scene.emplace<engine::ColliderComponent>(sphere, collider);
    engine::RigidBodyComponent rigid_body{};
    rigid_body.inverse_mass = 1.0f;
    scene.emplace<engine::RigidBodyComponent>(sphere, rigid_body);
  }

  engine::LogInfo("Collision demo: spheres fall into a walled pen. WASD/arrows move the camera.");

  engine::Timer timer;
  timer.Reset();
  engine::Input input;

  float accumulator = 0.0f;
  const float kFixedDt = scene.GetPhysicsSettings().fixed_dt;
  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();
    float frame_dt = std::min(delta_time, 0.25f);
    accumulator += frame_dt;

    window.PollEvents(input);
    scene.HandleInput(input, delta_time);
    scene.Update(delta_time);
    while (accumulator >= kFixedDt) {
      scene.FixedUpdate(kFixedDt);
      accumulator -= kFixedDt;
  }
  const float alpha = accumulator / kFixedDt;

    renderer.BeginFrame();
    scene.Render(renderer, alpha);
    renderer.EndFrame();
  }

  renderer.Shutdown();
  window.Shutdown();
  engine::LogInfo("Collision demo complete.");
  return 0;
}
