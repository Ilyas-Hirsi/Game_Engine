#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Texture.h"

struct SDL_Renderer;
struct SDL_Window;
namespace engine {

class Window;
using SDL_GLContext = void*;

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

  TextureHandle LoadTexture(const std::string& texture_path);

  void BeginFrame();
  void EndFrame();
  void DrawSprite(TextureHandle texture, float x, float y, int width, int height);


 private:
  SDL_GLContext gl_context_ = nullptr;
  bool image_initialized_ = false;
  std::vector<unsigned int> textures_;
  std::unordered_map<std::string, TextureHandle> texture_cache_;
  SDL_Window* window_ = nullptr;
  unsigned int shader_program_ = 0;
  unsigned int vertex_array_ = 0;
  unsigned int vertex_buffer_ = 0;
  unsigned int index_buffer_ = 0;
  int viewport_width_ = 0;
  int viewport_height_ = 0;
  std::uint8_t clear_r_ = 32;
  std::uint8_t clear_g_ = 32;
  std::uint8_t clear_b_ = 48;
  std::uint8_t clear_a_ = 255;
};

}  // namespace engine
