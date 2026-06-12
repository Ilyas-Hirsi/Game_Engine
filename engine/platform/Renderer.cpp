#include "Renderer.h"

#include "Window.h"

#include <glad/glad.h>
#include <SDL.h>
#include <SDL_image.h>

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

  if (renderer_ != nullptr && (IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != 0) {
    image_initialized_ = true;
  }

  return renderer_ != nullptr;
}

void Renderer::Shutdown() {
  for (SDL_Texture* texture : textures_) {
    SDL_DestroyTexture(texture);
  }
  textures_.clear();
  texture_cache_.clear();

  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (image_initialized_) {
    IMG_Quit();
    image_initialized_ = false;
  }
}

void Renderer::SetClearColor(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                             std::uint8_t a) {
  clear_r_ = r;
  clear_g_ = g;
  clear_b_ = b;
  clear_a_ = a;
}

TextureHandle Renderer::LoadTexture(const std::string& texture_path) {
  const auto existing = texture_cache_.find(texture_path);
  if (existing != texture_cache_.end()) {
    return existing->second;
  }

  if (renderer_ == nullptr) {
    return {};
  }

  SDL_Surface* surface = IMG_Load(texture_path.c_str());
  if (surface == nullptr) {
    return {};
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
  SDL_FreeSurface(surface);
  if (texture == nullptr) {
    return {};
  }

  textures_.push_back(texture);
  TextureHandle handle{static_cast<std::uint32_t>(textures_.size())};
  texture_cache_[texture_path] = handle;
  return handle;
}

void Renderer::BeginFrame() {
  SDL_SetRenderDrawColor(renderer_, clear_r_, clear_g_, clear_b_, clear_a_);
  SDL_RenderClear(renderer_);
}

void Renderer::EndFrame() { SDL_RenderPresent(renderer_); }

void Renderer::DrawSprite(TextureHandle texture, float x, float y, int width, int height) {
  if (renderer_ == nullptr || !texture.IsValid() || texture.id > textures_.size()) {
    return;
  }

  SDL_Texture* native_texture = textures_[texture.id - 1];
  SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), width, height};
  SDL_RenderCopy(renderer_, native_texture, nullptr, &rect);
}
SDL_Renderer* Renderer::GetNativeRenderer() const { return renderer_; }

}  // namespace engine
