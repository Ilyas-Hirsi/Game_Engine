#include "Scene/Components/InputComponent.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/SpriteComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Scene.h"
#include "core/Log.h"
#include "core/Timer.h"
#include "platform/Renderer.h"
#include "platform/Window.h"

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
  scene.emplace<engine::SpriteComponent>(monkey, monkey_sprite);

  // Unit cube: each vertex is position (xyz) + color (rgb), stride 6 floats.
  const std::vector<float> cube_vertices = {
      -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
       0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
       0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
      -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
      -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
       0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
       0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,
      -0.5f,  0.5f,  0.5f, 0.2f, 0.2f, 0.2f,
  };
  const std::vector<unsigned int> cube_indices = {
      0, 1, 2, 2, 3, 0,  // back
      4, 5, 6, 6, 7, 4,  // front
      0, 4, 7, 7, 3, 0,  // left
      1, 5, 6, 6, 2, 1,  // right
      3, 2, 6, 6, 7, 3,  // top
      0, 1, 5, 5, 4, 0,  // bottom
  };
  const engine::MeshHandle cube_mesh =
      renderer.CreateMesh(cube_vertices, cube_indices);
  if (!cube_mesh.IsValid()) {
    engine::LogError("Failed to create cube mesh.");
    return 1;
  }

  engine::Entity& cube = scene.CreateEntity("Cube");

  engine::TransformComponent cube_transform{};
  cube_transform.x = 0.0f;
  cube_transform.y = 0.0f;
  cube_transform.z = 0.0f;
  cube_transform.rotation_x = 25.0f;
  cube_transform.rotation_y = 35.0f;
  scene.emplace<engine::TransformComponent>(cube, cube_transform);

  engine::MeshComponent cube_mesh_component{};
  cube_mesh_component.mesh = cube_mesh;
  scene.emplace<engine::MeshComponent>(cube, cube_mesh_component);

  float fps_timer = 0.0f;
  int frame_count = 0;
  engine::Input input;
  engine::LogInfo("Sandbox running. Press Escape or close the window to quit.");

  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();

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
