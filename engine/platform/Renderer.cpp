#include "Renderer.h"

#include "Window.h"

#include <SDL.h>

namespace engine {

Renderer::~Renderer() { Shutdown(); }

bool Renderer::Initialize(Window& window) {
  if (renderer_ != nullptr){
    return true;
  }
  SDL_Window* native_window = window.GetNativeWindow();
  if (native_window == nullptr) {
    return false;
  }

  renderer_ = SDL_CreateRenderer(
      native_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer_ == nullptr) {
    renderer_ = SDL_CreateRenderer(native_window, -1, SDL_RENDERER_SOFTWARE);
  }

  return renderer_ != nullptr;
}

void Renderer::Shutdown() {
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
}

void Renderer::SetClearColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                             std::uint8_t a) {
  clear_r_ = r;
  clear_g_ = g;
  clear_b_ = b;
  clear_a_ = a;
}

void Renderer::BeginFrame() {
  SDL_SetRenderDrawColor(renderer_, clear_r_, clear_g_, clear_b_, clear_a_);
  SDL_RenderClear(renderer_);
}

void Renderer::EndFrame() { SDL_RenderPresent(renderer_); }

SDL_Renderer* Renderer::GetNativeRenderer() const { return renderer_; }

}  // namespace engine
