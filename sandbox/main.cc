#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/ColliderComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Events/CollisionEvents.h"
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
#include <string>
#include <vector>


// A character is one rigid body of primitive children: a sphere head, capsule
// torso, capsule arms (T-pose) and legs. One table drives both the render mesh
// and the collider, so visuals and physics can't drift apart. Sizes are world
// units; entities keep scale = 1 (see the MeshFactory capsule note).
struct BodyPart {
  bool is_sphere;
  float radius;
  float height;  // capsule cylinder section; ignored for spheres
  glm::vec3 offset;
  glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

static const glm::quat kArmRot =
    glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

static const BodyPart kCharacterParts[] = {
    {true,  0.8f, 0.0f, { 0.0f,  2.6f, 0.0f}},           // head
    {false, 1.0f, 2.0f, { 0.0f,  0.0f, 0.0f}},           // torso
    {false, 0.4f, 1.6f, {-1.7f,  0.6f, 0.0f}, kArmRot},  // left arm
    {false, 0.4f, 1.6f, { 1.7f,  0.6f, 0.0f}, kArmRot},  // right arm
    {false, 0.5f, 1.6f, {-0.55f, -2.5f, 0.0f}},          // left leg
    {false, 0.5f, 1.6f, { 0.55f, -2.5f, 0.0f}}           // right leg
};

static engine::MeshData BuildCharacterMesh() {
  engine::MeshData mesh;
  for (const BodyPart& part : kCharacterParts) {
    const engine::MeshData prim = part.is_sphere
        ? engine::MeshFactory::Sphere(16, part.radius)
        : engine::MeshFactory::Capsule(16, part.radius, part.height);
    engine::MeshFactory::Append(mesh, prim, part.offset, part.rotation);
  }
  return mesh;
}

static engine::ColliderComponent BuildCharacterCollider() {
  engine::ColliderComponent collider;
  for (const BodyPart& part : kCharacterParts) {
    engine::ChildShape child;
    if (part.is_sphere) {
      child.shape = engine::Sphere{part.radius};
    } else {
      child.shape = engine::Capsule{part.radius, part.height};
    }
    child.local_position = part.offset;
    child.local_rotation = part.rotation;
    collider.child_shapes.push_back(child);
  }
  return collider;
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  // Arena is a +/-kHalf box. The floor sits at y = -kHalf; four invisible
  // wall planes face inward so nothing escapes sideways.
  const float kHalf       = 100.0f;
  const float kBodyExtent = 4.0f;  // spawn margin: roughly a character's reach
  // Each character is 6 primitives, so entity-pair narrow phase is ~36 child
  // tests; fewer, larger bodies than the old sphere demo keeps the cost sane.
  const int   kBodyCount  = 500;

  // Lifetime churn: each character lives a few seconds, then is destroyed and
  // respawned from the spawner, holding the population at kBodyCount while
  // entity IDs are recycled.
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
  scene.GetEventBus().Subscribe<engine::CollisionEvent>(
    [](const engine::CollisionEvent& e) {
      
    });
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
  floor_collider.child_shapes = {{engine::Plane{glm::vec3(0.0f, 1.0f, 0.0f), -kHalf}}};
  floor_collider.is_static = true;
  scene.emplace<engine::ColliderComponent>(floor, floor_collider);

  // --- Containment walls (invisible static plane colliders) -----------------
  auto add_wall = [&](glm::vec3 normal) {
    engine::Entity wall = scene.CreateEntity();
    scene.emplace<engine::TransformComponent>(wall, engine::TransformComponent{});
    engine::ColliderComponent collider{};
    collider.child_shapes = {{engine::Plane{normal, -kHalf}}};
    collider.is_static = true;
    scene.emplace<engine::ColliderComponent>(wall, collider);
  };
  add_wall(glm::vec3( 1.0f, 0.0f,  0.0f));  // left  (x >= -kHalf)
  add_wall(glm::vec3(-1.0f, 0.0f,  0.0f));  // right (x <=  kHalf)
  add_wall(glm::vec3( 0.0f, 0.0f,  1.0f));  // back  (z >= -kHalf)
  add_wall(glm::vec3( 0.0f, 0.0f, -1.0f));  // front (z <=  kHalf)

  // --- Falling bodies: compound character ragdolls --------------------------
  // One baked mesh shared by every character; the compound collider mirrors
  // it part-for-part via kCharacterParts.
  const engine::MeshHandle character_mesh =
      renderer.CreateMesh(BuildCharacterMesh());
  const engine::TextureHandle crate_tex =
      renderer.LoadTexture("C:/Users/ilyas/Downloads/mauga.jpeg");  // path to test texture

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<float> spread(-kHalf + kBodyExtent, kHalf - kBodyExtent);
  std::uniform_real_distribution<float> height(10.0f, 80.0f);
  std::uniform_real_distribution<float> lifetime_dist(kLifetimeMin, kLifetimeMax);

  auto spawn_body = [&](const glm::vec3& position) -> engine::Entity {
    engine::Entity body = scene.CreateEntity();

    engine::TransformComponent transform{};
    transform.position = position;
    // Character sizes are baked into the mesh and collider; scaling the
    // transform would desync them (and break the capsules), so stay at 1.
    transform.scale = glm::vec3(1.0f);
    scene.emplace<engine::TransformComponent>(body, transform);

    engine::MeshComponent mesh{};
    mesh.mesh = character_mesh;
    mesh.entity_id = body.GetId();
    scene.emplace<engine::MeshComponent>(body, mesh);
    scene.emplace<engine::TextureComponent>(
      body, engine::TextureComponent{crate_tex});

    scene.emplace<engine::ColliderComponent>(body, BuildCharacterCollider());

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
  // respawn so the vector never grows past kBodyCount.
  struct TimedBody {
    engine::Entity entity;
    float remaining;
  };
  std::vector<TimedBody> bodies;
  bodies.reserve(kBodyCount);
  for (int i = 0; i < kBodyCount; ++i) {
    const glm::vec3 position(spread(rng), height(rng), spread(rng));
    bodies.push_back({spawn_body(position), lifetime_dist(rng)});
  }

  engine::LogInfo(
      "Collision demo: compound character ragdolls fall into a walled pen, expire after "
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

    // Expire and respawn bodies at the spawner's position. Runs outside any
    // view iteration, so destroying here is safe.
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
