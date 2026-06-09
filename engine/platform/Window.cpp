#include "Window.h"

#include <SDL.h>

#include <algorithm>

namespace engine {

  Window::Window(const std::string& name, int width, int height)
    : width_(width), height_(height), title_(name) {
    Initialize(width, height, name);
  }

  Window::~Window() {
    Shutdown();
  }
bool Window::Initialize(int width, int height, const std::string& title) {
  width_ = std::max(1, std::min(width, 16384));
  height_ = std::max(1, std::min(height, 16384));
  should_close_ = false;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    return false;
  }
  sdl_initialized_ = true;

  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width_, height_, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (window_ == nullptr) {
    Shutdown();
    return false;
  }

  return true;
}

void Window::Shutdown() {
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }

  if (sdl_initialized_) {
    SDL_Quit();
    sdl_initialized_ = false;
  }
}

void Window::PollEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        should_close_ = true;
        break;
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
          case SDL_WINDOWEVENT_CLOSE:
            should_close_ = true;
            break;
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            width_ = event.window.data1;
            height_ = event.window.data2;
            break;
          default:
            break;
        }
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          should_close_ = true;
        }
        break;
      default:
        break;
    }
  }
}

bool Window::ShouldClose() const { return should_close_; }

int Window::Width() const { return width_; }

int Window::Height() const { return height_; }

SDL_Window* Window::GetNativeWindow() const { return window_; }

}  // namespace engine
