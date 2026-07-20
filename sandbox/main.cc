#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/ColliderComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/TextureComponent.h"
#include "Scene/Scene.h"
#include "core/Log.h"
#include "core/Timer.h"
#include "platform/MeshFactory.h"
#include "platform/Renderer.h"
#include "platform/Window.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>


int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  // Arena is a +/-kHalf box. The floor sits at y = -kHalf; four invisible
  // wall planes face inward so nothing escapes sideways.
  const float kHalf        = 100.0f;
  const float kSphereScale = 4.0f;                 // mesh radius 0.5 -> visual 2.0
  const float kRadius      = 0.5f * kSphereScale;  // collider radius matches
  const int   kSphereCount = 1500;

  // Lifetime churn: every sphere lives a few seconds, then is destroyed and
  // replaced by a fresh one dropped from the spawner, so the population stays
  // at kSphereCount while entity IDs are continuously destroyed and recycled.
  const float kLifetimeMin      = 4.0f;
  const float kLifetimeMax      = 10.0f;
  const float kSpawnHeight      = 80.0f;
  const float kSpawnerRadius    = 60.0f;  // spawner sweeps a circle above the pen
  const float kSpawnerAngSpeed  = 0.6f;   // radians per second

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

  // --- Falling bodies: a random mix of spheres and cubes --------------------
  const engine::MeshHandle sphere_mesh =
      renderer.CreateMesh(engine::MeshFactory::Sphere());
  const engine::MeshHandle cube_mesh =
      renderer.CreateMesh(engine::MeshFactory::Cube());
  const engine::MeshHandle capsule_mesh =
      renderer.CreateMesh(engine::MeshFactory::Capsule());
  const engine::TextureHandle crate_tex =
      renderer.LoadTexture("C:/Users/ilyas/Downloads/mauga.jpeg");  // path to test texture

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<float> spread(-kHalf + kRadius, kHalf - kRadius);
  std::uniform_real_distribution<float> height(10.0f, 80.0f);
  std::uniform_real_distribution<float> lifetime_dist(kLifetimeMin, kLifetimeMax);
  std::bernoulli_distribution coin(0.5);  // true = cube, false = sphere

  auto spawn_body = [&](const glm::vec3& position) -> engine::Entity {
    const bool is_cube = coin(rng);
    engine::Entity body = scene.CreateEntity();

    engine::TransformComponent transform{};
    transform.position = position;
    transform.scale = glm::vec3(kSphereScale);
    scene.emplace<engine::TransformComponent>(body, transform);

    engine::MeshComponent mesh{};
    mesh.mesh = is_cube ? capsule_mesh : sphere_mesh;
    mesh.entity_id = body.GetId();
    scene.emplace<engine::MeshComponent>(body, mesh);
    scene.emplace<engine::TextureComponent>(
      body, engine::TextureComponent{crate_tex});

    // Unit meshes scaled by kSphereScale: sphere radius and cube half extents
    // both come out to kRadius, so the colliders match the visuals.
    engine::ColliderComponent collider{};
    if (is_cube) {
      collider.shape = engine::Capsule{kRadius, 4.0f};
    } else {
      collider.shape = engine::Sphere{kRadius};
    }
    scene.emplace<engine::ColliderComponent>(body, collider);

    engine::RigidBodyComponent rigid_body{};
    rigid_body.inverse_mass = 1.0f;
    rigid_body.inverse_inertia = glm::vec3(0.1f, 0.1f, 0.1f);
    // Seed the interpolation history at the spawn point, otherwise the first
    // rendered frame lerps from the recycled slot's old position (or origin).
    rigid_body.previous_position = position;
    scene.emplace<engine::RigidBodyComponent>(body, rigid_body);
    return body;
  };

  // Each body carries its handle + remaining lifetime; slots are reused on
  // respawn so the vector never grows past kSphereCount.
  struct TimedBody {
    engine::Entity entity;
    float remaining;
  };
  std::vector<TimedBody> bodies;
  bodies.reserve(kSphereCount);
  for (int i = 0; i < kSphereCount; ++i) {
    const glm::vec3 position(spread(rng), height(rng), spread(rng));
    bodies.push_back({spawn_body(position), lifetime_dist(rng)});
  }

  engine::LogInfo(
      "Collision demo: spheres and cubes fall into a walled pen, expire after "
      "a few seconds, and respawn from the orbiting spawner. WASD/arrows move "
      "the camera.");

  engine::Timer timer;
  timer.Reset();
  engine::Input input;

  float accumulator = 0.0f;
  float spawner_angle = 0.0f;
  const float kFixedDt = scene.GetPhysicsSettings().fixed_dt;
  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();
    float frame_dt = std::min(delta_time, 0.25f);
    accumulator += frame_dt;

    window.PollEvents(input);
    scene.HandleInput(input, frame_dt);
    scene.Update(frame_dt);

    // Lifetime churn: expire spheres and respawn them at the spawner's
    // current position. This runs outside any view iteration, so destroying
    // here is safe (no deferred-destroy queue needed yet).
    spawner_angle += kSpawnerAngSpeed * frame_dt;
    const glm::vec3 spawn_pos(std::cos(spawner_angle) * kSpawnerRadius,
                              kSpawnHeight,
                              std::sin(spawner_angle) * kSpawnerRadius);
    for (TimedBody& body : bodies) {
      body.remaining -= frame_dt;
      if (body.remaining > 0.0f) continue;
      scene.DestroyEntity(body.entity);
      body.entity = spawn_body(spawn_pos);
      body.remaining = lifetime_dist(rng);
    }

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
