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

#include <algorithm>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <string>

// Cube stress test.
int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  //  Stress test knobs 
  const int   kCubesPerFrame = 1000;     // how many cubes to spawn each frame
  const float kTargetFps     = 60.0f;   
  const float kWarmupSeconds = 1.0f;   
  const int   kMaxCubes      = 100000;  // hard cap so a fast GPU can't OOM us
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
  const engine::MeshData cube_mesh_data = engine::MeshFactory::Sphere(16);
  const engine::MeshHandle cube_mesh = renderer.CreateMesh(cube_mesh_data);
  const engine::TextureHandle cube_texture = renderer.LoadTexture("C:/Users/ilyas/SumMonkey/Assets/Sprites/Monkey/Idle/monkey-idle_-1.png");

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
    engine::TextureComponent texture{};
    texture.texture = cube_texture;
    scene.emplace<engine::TextureComponent>(cube, texture);
  };

  engine::Timer timer;
  timer.Reset();

  engine::Input input;

  // Frame metrics over one reporting window. FPS is derived from frames counted
  // against accumulated wall time (frames / seconds), which is the honest
  // throughput number. Reading 1/delta of a single frame is noisy and, with a
  // clamped delta, can report a fixed wrong value regardless of real speed.
  struct FrameStats {
    int   frames   = 0;
    float seconds  = 0.0f;
    float best_dt  = std::numeric_limits<float>::max();  // fastest frame
    float worst_dt = 0.0f;                               // slowest frame

    void Add(float dt) {
      ++frames;
      seconds += dt;
      best_dt = std::min(best_dt, dt);
      worst_dt = std::max(worst_dt, dt);
    }
    void Reset() { *this = FrameStats{}; }
    float Fps() const { return seconds > 0.0f ? frames / seconds : 0.0f; }
    float AvgMs() const { return frames > 0 ? seconds * 1000.0f / frames : 0.0f; }
    float BestMs() const { return frames > 0 ? best_dt * 1000.0f : 0.0f; }
    float WorstMs() const { return worst_dt * 1000.0f; }
  };

  FrameStats  stats;                       // accumulates within the window
  bool        growing       = true;
  int         cube_count    = 0;
  float       elapsed       = 0.0f;        // total run time (warmup gate)
  float       report_timer  = 0.0f;
  const float kReportInterval = 0.5f;      // seconds between reports

  engine::LogInfo(
      "Cube stress test running. NOTE: VSync is on, so FPS is capped at the "
      "monitor refresh rate (frame time floors near the refresh interval). The "
      "test grows the cube count until the windowed average FPS drops below the "
      "target.");

  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();
    elapsed += delta_time;

    window.PollEvents(input);
    scene.HandleInput(input, delta_time);

    if (growing) {
      for (int i = 0; i < kCubesPerFrame && cube_count < kMaxCubes; ++i) {
        spawn_cube();
        ++cube_count;
      }
    }

    scene.Update(delta_time);

    renderer.BeginFrame();
    scene.Render(renderer);
    renderer.EndFrame();

    stats.Add(delta_time);
    report_timer += delta_time;
    if (report_timer >= kReportInterval) {
      std::ostringstream stream;
      stream << std::fixed << std::setprecision(1)
             << (growing ? "[growing] " : "[stable]  ")
             << "cubes: " << cube_count
             << "  fps: " << stats.Fps()
             << "  frame ms avg/best/worst: " << stats.AvgMs() << " / "
             << stats.BestMs() << " / " << stats.WorstMs();
      engine::LogInfo(stream.str());

      // End the growth phase once the windowed average can no longer hold the
      // target (after a warmup so startup hitches don't trip it early).
      if (growing) {
        const bool fps_dropped =
            elapsed > kWarmupSeconds && stats.Fps() < kTargetFps;
        if (fps_dropped || cube_count >= kMaxCubes) {
          growing = false;
          std::ostringstream result;
          result << std::fixed << std::setprecision(1)
                 << "RESULT: sustained " << cube_count << " cubes at "
                 << stats.Fps() << " fps (avg frame " << stats.AvgMs()
                 << " ms, worst " << stats.WorstMs() << " ms, target "
                 << kTargetFps << " fps).";
          engine::LogInfo(result.str());
        }
      }

      stats.Reset();
      report_timer = 0.0f;
    }
  }

  renderer.Shutdown();
  window.Shutdown();
  engine::LogInfo("Stress test complete.");
  return 0;
}
