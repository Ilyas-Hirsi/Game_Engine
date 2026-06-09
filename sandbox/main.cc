#include "core/Log.h"
#include "core/Timer.h"
#include "platform/Renderer.h"
#include "platform/Window.h"

#include <sstream>
#include <string>

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

  float fps_timer = 0.0f;
  int frame_count = 0;

  engine::LogInfo("Sandbox running. Press Escape or close the window to quit.");

  while (!window.ShouldClose()) {
    const float delta_time = timer.Tick();

    window.PollEvents();
    renderer.BeginFrame();
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
