#pragma once

namespace engine {

class Window;
class Renderer;

// Owns the ImGui context and its SDL2 + OpenGL3 backends.
class ImGuiLayer {
 public:
  ImGuiLayer() = default;
  ~ImGuiLayer();

  ImGuiLayer(const ImGuiLayer&) = delete;
  ImGuiLayer& operator=(const ImGuiLayer&) = delete;

  
  bool Init(Window& window, Renderer& renderer);
  void Shutdown();

  void BeginFrame();
  void EndFrame();

  bool IsInitialized() const { return initialized_; }

 private:
  bool initialized_ = false;
};

}  // namespace engine
