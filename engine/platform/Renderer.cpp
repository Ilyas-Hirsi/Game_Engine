#include "Renderer.h"

#include "Window.h"

#include <glad/glad.h>
#include <SDL.h>
#include <SDL_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

namespace {

constexpr char kVertexShaderSource[] = R"(
#version 330 core
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_tex_coord;

out vec2 v_tex_coord;

void main() {
  v_tex_coord = a_tex_coord;
  gl_Position = vec4(a_position, 0.0, 1.0);
}
)";
constexpr char kVertexShaderSourceDepth[] = R"(
  #version 330 core
  layout (location = 0) in vec3 a_position;
  layout (location = 1) in vec3 a_color;
  layout (location = 2) in mat4 a_model;   // per-instance, consumes locations 2,3,4,5
  layout (location = 6) in vec2 a_tex_coord;
  out vec3 v_color;
  out vec2 v_uv;
  uniform mat4 u_viewProj;
  void main() {
    v_color = a_color;
    v_uv = a_tex_coord;
    gl_Position = u_viewProj * a_model * vec4(a_position, 1.0);
  }
  )";

constexpr char kFragmentShaderSource[] = R"(
#version 330 core
in vec2 v_tex_coord;

out vec4 fragment_color;

uniform sampler2D u_texture;

void main() {
  fragment_color = texture(u_texture, v_tex_coord);
}
)";

constexpr char kFragmentShaderSourceDepth[] = R"(
  #version 330 core
  in vec3 v_color;
  in vec2 v_uv;
  out vec4 fragment_color;
  uniform sampler2D u_texture;
  uniform int u_useTexture;
  void main() {
    if (u_useTexture != 0) {
      fragment_color = texture(u_texture, v_uv) * vec4(v_color, 1.0);
    } else {
      fragment_color = vec4(v_color, 1.0);
    }
  }
  )";
GLuint CompileShader(GLenum type, const char* source) {
  const GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE) {
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint CreateShaderProgram(const char* vertex_source, const char* fragment_source) {
  const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source);
  if (vertex_shader == 0) {
    return 0;
  }

  const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
  if (fragment_shader == 0) {
    glDeleteShader(vertex_shader);
    return 0;
  }

  const GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == GL_FALSE) {
    glDeleteProgram(program);
    return 0;
  }

  return program;
}

}  // namespace

Renderer::~Renderer() { Shutdown(); }

bool Renderer::Initialize(Window& window) {
  window_ = window.GetNativeWindow();
  viewport_width_ = window.Width();
  viewport_height_ = window.Height();

  if (gl_context_ != nullptr) {
    return true;
  }
  if (window_ == nullptr) {
    return false;
  }
  gl_context_ = SDL_GL_CreateContext(window_);
  if (gl_context_ == nullptr) {
    return false;
  }

  const bool glad_loaded =
      gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)) != 0;
  if (!glad_loaded) {
    SDL_GL_DeleteContext(gl_context_);
    gl_context_ = nullptr;
    return false;
  }

  SDL_GL_SetSwapInterval(1);
  glViewport(0, 0, viewport_width_, viewport_height_);

  shader_program_ = CreateShaderProgram(kVertexShaderSource, kFragmentShaderSource);
  if (shader_program_ == 0) {
    Shutdown();
    return false;
  }
  
  mesh_shader_program_ =
      CreateShaderProgram(kVertexShaderSourceDepth, kFragmentShaderSourceDepth);
  if (mesh_shader_program_ == 0) {
    Shutdown();
    return false;
  }
  u_viewproj_loc_ = glGetUniformLocation(mesh_shader_program_, "u_viewProj");
  glGenBuffers(1, &instance_vbo_);

  glGenVertexArrays(1, &vertex_array_);
  glGenBuffers(1, &vertex_buffer_);
  glGenBuffers(1, &index_buffer_);

  glBindVertexArray(vertex_array_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);

  const unsigned int indices[] = {0, 1, 2, 2, 3, 0};
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4,
                        reinterpret_cast<void*>(sizeof(float) * 2));

  glBindVertexArray(0);
  glEnable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != 0) {
    image_initialized_ = true;
  }

  const float aspect =
      static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
  projection_matrix_ =
      glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
  view_matrix_ = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));

  return true;
}

void Renderer::Shutdown() {
  for (unsigned int texture : textures_) {
    glDeleteTextures(1, &texture);
  }
  for (Mesh& mesh : meshes_) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.ebo);
  }
  meshes_.clear();
  textures_.clear();

  if (index_buffer_ != 0) {
    glDeleteBuffers(1, &index_buffer_);
    index_buffer_ = 0;
  }
  if (vertex_buffer_ != 0) {
    glDeleteBuffers(1, &vertex_buffer_);
    vertex_buffer_ = 0;
  }
  if (instance_vbo_ != 0) {
    glDeleteBuffers(1, &instance_vbo_);
    instance_vbo_ = 0;
  }
  if (vertex_array_ != 0) {
    glDeleteVertexArrays(1, &vertex_array_);
    vertex_array_ = 0;
  }
  if (shader_program_ != 0) {
    glDeleteProgram(shader_program_);
    shader_program_ = 0;
  }
  if (mesh_shader_program_ != 0){
    glDeleteProgram(mesh_shader_program_);
    mesh_shader_program_ = 0;
  }

  if (gl_context_ != nullptr) {
    SDL_GL_DeleteContext(gl_context_);
    gl_context_ = nullptr;
  }
  texture_cache_.clear();

  if (image_initialized_) {
    IMG_Quit();
    image_initialized_ = false;
  }

  window_ = nullptr;
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

  if (gl_context_ == nullptr) {
    return {};
  }

  SDL_Surface* surface = IMG_Load(texture_path.c_str());
  if (surface == nullptr) {
    return {};
  }

  SDL_Surface* formatted_surface =
      SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);
  if (formatted_surface == nullptr) {
    return {};
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, formatted_surface->w,
               formatted_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               formatted_surface->pixels);
  glBindTexture(GL_TEXTURE_2D, 0);

  SDL_FreeSurface(formatted_surface);

  textures_.push_back(texture);
  TextureHandle handle{static_cast<std::uint32_t>(textures_.size())};
  texture_cache_[texture_path] = handle;
  return handle;
}
MeshHandle Renderer::CreateMesh(const std::vector<float>& vertices,
  const std::vector<unsigned int>& indices) {
Mesh mesh;
mesh.index_count = static_cast<int>(indices.size());
glGenVertexArrays(1, &mesh.vao);
glGenBuffers(1, &mesh.vbo);
glGenBuffers(1, &mesh.ebo);
glBindVertexArray(mesh.vao);
glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
vertices.data(), GL_STATIC_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
indices.data(), GL_STATIC_DRAW);

glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
glEnableVertexAttribArray(1);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
    reinterpret_cast<void*>(3 * sizeof(float)));
glBindVertexArray(0);
meshes_.emplace_back(mesh);
return MeshHandle{static_cast<std::uint32_t>(meshes_.size())};  // 1-based, like textures
}

void Renderer::BeginFrame() {
  glClearColor(clear_r_/255.0f, clear_g_/255.0f, clear_b_/255.0f, clear_a_/255.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClear(GL_COLOR_BUFFER_BIT);

}

void Renderer::EndFrame() {
  if (window_ != nullptr) {
    SDL_GL_SwapWindow(window_);
  }
}

void Renderer::DrawSprite(TextureHandle texture, float x, float y, int width, int height) {
  if (gl_context_ == nullptr || shader_program_ == 0 || vertex_array_ == 0 ||
      !texture.IsValid() || texture.id > textures_.size() ||
      viewport_width_ <= 0 || viewport_height_ <= 0) {
    return;
  }

  const float left = (x / static_cast<float>(viewport_width_)) * 2.0f - 1.0f;
  const float right =
      ((x + static_cast<float>(width)) / static_cast<float>(viewport_width_)) *
          2.0f -
      1.0f;
  const float top = 1.0f - (y / static_cast<float>(viewport_height_)) * 2.0f;
  const float bottom =
      1.0f -
      ((y + static_cast<float>(height)) / static_cast<float>(viewport_height_)) *
          2.0f;

  const float vertices[] = {
      left,  top,    0.0f, 0.0f,
      right, top,    1.0f, 0.0f,
      right, bottom, 1.0f, 1.0f,
      left,  bottom, 0.0f, 1.0f,
  };

  glUseProgram(shader_program_);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures_[texture.id - 1]);
  glUniform1i(glGetUniformLocation(shader_program_, "u_texture"), 0);

  glBindVertexArray(vertex_array_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
  }

void Renderer::SetCamera(const glm::mat4& view, const glm::mat4& projection) {
  view_matrix_ = view;
  projection_matrix_ = projection;
}

float Renderer::GetAspectRatio() const {
  if (viewport_width_ <= 0 || viewport_height_ <= 0) {
    return 1.0f;
  }
  return static_cast<float>(viewport_width_) /
         static_cast<float>(viewport_height_);
}

void Renderer::DrawMesh(MeshHandle mesh, TextureHandle texture,const glm::mat4& model){
  (void)texture;  // add !texture.IsValid() || texture.id > textures_.size() when textures are ready to be used
  if (gl_context_ == nullptr || mesh_shader_program_ == 0 ||
      !mesh.IsValid() || mesh.id > meshes_.size() ||
      viewport_width_ <= 0 || viewport_height_ <= 0) {
    return;
  }

  const Mesh& mesh_data = meshes_[mesh.id - 1];
  const glm::mat4 mvp = projection_matrix_ * view_matrix_ * model;

  glUseProgram(mesh_shader_program_);
  glUniformMatrix4fv(
      glGetUniformLocation(mesh_shader_program_, "u_mvp"),
      1,
      GL_FALSE,
      glm::value_ptr(mvp));

  glBindVertexArray(mesh_data.vao);
  glDrawElements(GL_TRIANGLES, mesh_data.index_count, GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
 }


 MeshHandle Renderer::CreateMesh(const MeshData& mesh_data) {
  Mesh mesh;
  mesh.index_count = static_cast<int>(mesh_data.indices.size());
  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);
  glBindVertexArray(mesh.vao);

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh_data.vertices.size() * sizeof(float),
               mesh_data.vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               mesh_data.indices.size() * sizeof(unsigned int),
               mesh_data.indices.data(), GL_STATIC_DRAW);


               const GLsizei stride =
      static_cast<GLsizei>(mesh_data.Stride() * sizeof(float));
  std::size_t offset = 0;
  for (const VertexAttribute& attr : mesh_data.layout) {
    glEnableVertexAttribArray(attr.location);
    glVertexAttribPointer(attr.location, attr.component_count, GL_FLOAT,
                          GL_FALSE, stride,
                          reinterpret_cast<void*>(offset));
    offset += static_cast<std::size_t>(attr.component_count) * sizeof(float);
  }

  // Per-instance model matrix: a mat4 occupies attribute locations 2..5, each
  // a vec4, sourced from the shared instance buffer and advancing once per
  // instance (divisor 1) rather than per vertex.
  glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_);
  for (unsigned int i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(2 + i);
    glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          reinterpret_cast<void*>(i * sizeof(glm::vec4)));
    glVertexAttribDivisor(2 + i, 1);
  }

  glBindVertexArray(0);
  meshes_.emplace_back(mesh);
  return MeshHandle{static_cast<std::uint32_t>(meshes_.size())};
  }

  void Renderer::DrawMeshInstanced(MeshHandle mesh, TextureHandle texture, const std::vector<glm::mat4>& models){
    if (models.empty() || mesh.id > meshes_.size()) return;
    const Mesh& m = meshes_[mesh.id - 1];
    glUseProgram(mesh_shader_program_);
    const glm::mat4 view_proj = projection_matrix_ * view_matrix_;
    glUniformMatrix4fv(u_viewproj_loc_, 1, GL_FALSE, glm::value_ptr(view_proj));
  
    const bool use_texture = texture.IsValid() && texture.id <= textures_.size();
    glUniform1i(glGetUniformLocation(mesh_shader_program_, "u_useTexture"),
                use_texture ? 1 : 0);
    if (use_texture) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textures_[texture.id - 1]);
      glUniform1i(glGetUniformLocation(mesh_shader_program_, "u_texture"), 0);
    }
  
    glBindBuffer(GL_ARRAY_BUFFER, instance_vbo_);
    glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4),
                 models.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(m.vao);
    glDrawElementsInstanced(GL_TRIANGLES, m.index_count,
                            GL_UNSIGNED_INT, nullptr,
                            static_cast<GLsizei>(models.size()));
    glBindVertexArray(0);
  }
}  // namespace engine