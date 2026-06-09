#pragma once

#include <cstdint>

struct SDL_Renderer;

namespace engine {

class Window;

class Renderer {
 public:
  Renderer() = default;
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  bool Initialize(Window& window);
  void Shutdown();

  void SetClearColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                     std::uint8_t a = 255);

  void BeginFrame();
  void EndFrame();

  SDL_Renderer* GetNativeRenderer() const;

 private:
  SDL_Renderer* renderer_ = nullptr;

  std::uint8_t clear_r_ = 32;
  std::uint8_t clear_g_ = 32;
  std::uint8_t clear_b_ = 48;
  std::uint8_t clear_a_ = 255;
};

}  // namespace engine
