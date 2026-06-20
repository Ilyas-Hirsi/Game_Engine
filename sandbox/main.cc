#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/SpriteComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"
#include "core/Log.h"
#include "core/Timer.h"
#include "platform/Renderer.h"
#include "platform/Window.h"
#include "platform/MeshFactory.h"
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  engine::Window window("Game Engine Sandbox", 1280, 720);

  engine::Renderer renderer;
  if (!renderer.Initialize(window)) {
    engine::LogError("Failed to initialize renderer.");
    return 1;
  }

  engine::Timer timer;
  timer.Reset();

  engine::Scene scene;
  engine::Entity& monkey = scene.CreateEntity("Monkey");

  engine::TransformComponent monkey_transform{};
  monkey_transform.x = 576.0f;
  monkey_transform.y = 296.0f;
  monkey_transform.velocity_x = 0.0f;
  monkey_transform.velocity_y = 0.0f;
  monkey_transform.scale_x = 1.0f;
  monkey_transform.scale_y = 1.0f;
  scene.emplace<engine::TransformComponent>(monkey, monkey_transform);

  engine::InputComponent monkey_input{};
  scene.emplace<engine::InputComponent>(monkey, monkey_input);

  const engine::TextureHandle monkey_texture = renderer.LoadTexture(
      R"(C:\Users\ilyas\SumMonkey\Assets\Sprites\Monkey\Idle\monkey-idle_-1.png)");
  if (!monkey_texture.IsValid()) {
    engine::LogError("Failed to load monkey texture.");
    return 1;
  }

  engine::SpriteComponent monkey_sprite{};
  monkey_sprite.texture = monkey_texture;
  monkey_sprite.width = 128;
  monkey_sprite.height = 128;
  monkey_sprite.rotation = 0;
  monkey_sprite.scale_x = 1;
  monkey_sprite.scale_y = 1;
  monkey_sprite.color_r = 255;


  engine::Entity& camera = scene.CreateEntity("Camera");

  engine::TransformComponent camera_transform{};
  camera_transform.x = 0.0f;
  camera_transform.y = 0.0f;
  camera_transform.z = 3.0f;
  scene.emplace<engine::TransformComponent>(camera, camera_transform);

  engine::CameraComponent camera_component{};
  camera_component.fov = 60.0f;
  camera_component.near_plane = 0.1f;
  camera_component.far_plane = 100.0f;
  scene.emplace<engine::CameraComponent>(camera, camera_component);
  engine::InputComponent camera_input{};
  camera_input.entity_id = camera.GetId();
  scene.emplace<engine::InputComponent>(camera, camera_input);

  engine::Entity& shape = scene.CreateEntity("Cube");
  engine::MeshData shape_mesh_data = engine::MeshFactory::Sphere();
  engine::MeshHandle shape_mesh = renderer.CreateMesh(shape_mesh_data);

  engine::TransformComponent shape_transform{};
  shape_transform.x = 0.0f;
  shape_transform.y = 0.0f;
  shape_transform.z = 0.0f;
  shape_transform.rotation_x = 25.0f;
  shape_transform.rotation_y = 35.0f;
  scene.emplace<engine::TransformComponent>(shape, shape_transform);

  engine::MeshComponent shape_mesh_component{};
  shape_mesh_component.mesh_data = shape_mesh_data;
  shape_mesh_component.mesh = shape_mesh;
  shape_mesh_component.entity_id = shape.GetId();
  scene.emplace<engine::MeshComponent>(shape, shape_mesh_component);

  float fps_timer = 0.0f;
  int frame_count = 0;
  engine::Input input;
  engine::LogInfo("Sandbox running. Press Escape or close the window to quit.");

  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();
    shape_transform.rotation_x += 20.0f * delta_time;
    window.PollEvents(input);
    scene.HandleInput(input, delta_time);
    scene.Update(delta_time);

    renderer.BeginFrame();
    scene.Render(renderer);
    renderer.EndFrame();

    frame_count++;
    fps_timer += delta_time;
    if (fps_timer >= 1.0f) {
      const float fps = static_cast<float>(frame_count) / fps_timer;
      std::ostringstream stream;
      stream << "FPS: " << fps << "  dt: " << timer.DeltaTime() << "s  size: "
             << window.Width() << 'x' << window.Height();
      engine::LogInfo(stream.str());

      fps_timer = 0.0f;
      frame_count = 0;
    }
  }

  renderer.Shutdown();
  window.Shutdown();
  engine::LogInfo("Sandbox shutdown complete.");
  return 0;
}
