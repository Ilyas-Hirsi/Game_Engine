#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"
#include "core/Log.h"
#include "core/Timer.h"
#include "platform/MeshFactory.h"
#include "platform/Renderer.h"
#include "platform/Window.h"

#include <random>
#include <sstream>
#include <string>

// Cube stress test.
int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  //  Stress test knobs 
  const int   kCubesPerFrame = 200;     // how many cubes to spawn each frame
  const float kTargetFps     = 60.0f;   
  const float kWarmupSeconds = 1.0f;   
  const int   kMaxCubes      = 500000;  // hard cap so a fast GPU can't OOM us
  const float kHalfExtent    = 50.0f;   // cubes spawn inside a +/-50 unit box
  // -

  engine::Window window("Cube Stress Test", 1280, 720);

  engine::Renderer renderer;
  if (!renderer.Initialize(window)) {
    engine::LogError("Failed to initialize renderer.");
    return 1;
  }

  engine::Scene scene;


  engine::Entity& camera = scene.CreateEntity();
  engine::TransformComponent camera_transform{};
  camera_transform.position = glm::vec3(0.0f, 0.0f, 120.0f);
  camera_transform.facing = glm::vec3(0.0f, 0.0f, -1.0f);
  camera_transform.velocity = 40.0f;
  scene.emplace<engine::TransformComponent>(camera, camera_transform);

  engine::CameraComponent camera_component{};
  camera_component.fov = 60.0f;
  camera_component.near_plane = 0.1f;
  camera_component.far_plane = 1000.0f;
  scene.emplace<engine::CameraComponent>(camera, camera_component);

  engine::InputComponent camera_input{};
  scene.emplace<engine::InputComponent>(camera, camera_input);


  // One shared cube mesh on the GPU. Every cube entity reuses this handle, so
  // the test measures per-entity iteration + draw-call cost, not buffer uploads.
  const engine::MeshData cube_mesh_data = engine::MeshFactory::Cube();
  const engine::MeshHandle cube_mesh = renderer.CreateMesh(cube_mesh_data);

  std::mt19937 rng(1337u);
  std::uniform_real_distribution<float> pos(-kHalfExtent, kHalfExtent);
  std::uniform_real_distribution<float> spin(0.0f, 360.0f);

  auto spawn_cube = [&]() {
    // Grab the reference and consume it immediately: the two emplace() calls use
    // cube.GetId() before any further CreateEntity() can invalidate it.
    engine::Entity& cube = scene.CreateEntity();
    engine::TransformComponent transform{};
    transform.position = glm::vec3(pos(rng), pos(rng), pos(rng));
    transform.rotation = glm::vec3(spin(rng), spin(rng), spin(rng));
    transform.scale = glm::vec3(1.0f);
    scene.emplace<engine::TransformComponent>(cube, transform);

    engine::MeshComponent mesh{};
    mesh.mesh = cube_mesh;
    mesh.entity_id = cube.GetId();
    scene.emplace<engine::MeshComponent>(cube, mesh);
  };

  engine::Timer timer;
  timer.Reset();

  engine::Input input;

  int   cube_count   = 0;
  bool  growing      = true;
  float smoothed_fps = kTargetFps;  // exponential moving average of FPS
  float elapsed      = 0.0f;
  float report_timer = 0.0f;

  engine::LogInfo(
      "Cube stress test running. Spawning cubes until FPS drops below target.");

  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();
    elapsed += delta_time;

    window.PollEvents(input);
    scene.HandleInput(input, delta_time);

    // Smooth FPS so a single hitch doesn't prematurely end the test.
    if (delta_time > 0.0f) {
      const float instant_fps = 1.0f / delta_time;
      smoothed_fps = smoothed_fps * 0.9f + instant_fps * 0.1f;
    }

    if (growing) {
      for (int i = 0; i < kCubesPerFrame && cube_count < kMaxCubes; ++i) {
        spawn_cube();
        ++cube_count;
      }
      const bool fps_exceeded =
          elapsed > kWarmupSeconds && smoothed_fps < kTargetFps;
      if (fps_exceeded || cube_count >= kMaxCubes) {
        growing = false;
        std::ostringstream result;
        result << "RESULT: sustained " << cube_count << " cubes at "
               << smoothed_fps << " FPS (" << kCubesPerFrame
               << " spawned/frame, target " << kTargetFps << " FPS).";
        engine::LogInfo(result.str());
      }
    }

    scene.Update(delta_time);

    renderer.BeginFrame();
    scene.Render(renderer);
    renderer.EndFrame();

    // Periodic progress so you can watch the climb in the console.
    report_timer += delta_time;
    if (report_timer >= 0.25f) {
      std::ostringstream stream;
      stream << (growing ? "[growing] " : "[stable]  ") << "cubes: " << cube_count
             << "  FPS: " << smoothed_fps << "  dt: " << delta_time * 1000.0f
             << " ms";
      engine::LogInfo(stream.str());
      report_timer = 0.0f;
    }
  }

  renderer.Shutdown();
  window.Shutdown();
  engine::LogInfo("Stress test complete.");
  return 0;
}
