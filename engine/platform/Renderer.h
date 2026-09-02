#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Texture.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include "MeshData.h"

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
  MeshHandle CreateMesh(const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices);

  void BeginFrame();
  void EndFrame();
  void DrawSprite(TextureHandle texture, float x, float y, int width, int height);
  void DrawMesh(MeshHandle mesh, TextureHandle texture, const glm::mat4& model);
  void DrawMeshInstanced(MeshHandle mhes, TextureHandle texture, const std::vector<glm::mat4>& models);
  void CalculateMeshBounds(const MeshData& mesh_data, Mesh& mesh);
  void CalculateMeshBounds(const std::vector<float>& vertices, int stride, Mesh& mesh);
  MeshHandle CreateMesh(const MeshData& mesh_data);


  bool GetMeshBounds(MeshHandle mesh, glm::vec3& out_min, glm::vec3& out_max) const;

  SDL_GLContext GetGLContext() const { return gl_context_; }
  void SetCamera(const glm::mat4& view, const glm::mat4& projection);
  float GetAspectRatio() const;
  const glm::mat4& GetViewMatrix() const;
  const glm::mat4& GetProjectionMatrix() const;
  glm::mat4 GetViewProjectionMatrix() const;
 private:
  SDL_GLContext gl_context_ = nullptr;
  bool image_initialized_ = false;
  std::vector<unsigned int> textures_;
  std::vector<Mesh> meshes_;
  std::unordered_map<std::string, TextureHandle> texture_cache_;
  SDL_Window* window_ = nullptr;
  unsigned int shader_program_ = 0;
  unsigned int mesh_shader_program_ = 0;
  unsigned int vertex_array_ = 0;
  unsigned int vertex_buffer_ = 0;
  unsigned int index_buffer_ = 0;
  unsigned int instance_vbo_ = 0;  // shared per-instance model matrices for instanced meshes
  int viewport_width_ = 0;
  int viewport_height_ = 0;
  int u_viewproj_loc_ = -1;
  int u_use_texture_loc_ = -1;
  int u_sprite_texture_loc_ = -1;
  int u_mesh_texture_loc_ = -1;
  std::uint8_t clear_r_ = 32;
  std::uint8_t clear_g_ = 32;
  std::uint8_t clear_b_ = 48;
  std::uint8_t clear_a_ = 255;
  glm::mat4 view_matrix_{1.0f};
  glm::mat4 projection_matrix_{1.0f};
};

}  // namespace engine
