#pragma once

#include <string>

struct SDL_Window;

namespace engine {

class Window {
 public:
  Window(const std::string& name, int width, int height);
  ~Window();
  bool Initialize(int width, int height, const std::string& title);
  void Shutdown();

  void PollEvents();
  bool ShouldClose() const;

  int Width() const;
  int Height() const;

  SDL_Window* GetNativeWindow() const;

 private:
  SDL_Window* window_ = nullptr;
  int width_ = 0;
  std::string title_;
  int height_ = 0;
  bool should_close_ = false;
  bool sdl_initialized_ = false;
};

}  // namespace engine
