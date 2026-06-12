#include "Renderer.h"

#include "Window.h"

#include <glad/glad.h>
#include <SDL.h>
#include <SDL_image.h>

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

constexpr char kFragmentShaderSource[] = R"(
#version 330 core
in vec2 v_tex_coord;

out vec4 fragment_color;

uniform sampler2D u_texture;

void main() {
  fragment_color = texture(u_texture, v_tex_coord);
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

GLuint CreateShaderProgram() {
  const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, kVertexShaderSource);
  if (vertex_shader == 0) {
    return 0;
  }

  const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
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

  shader_program_ = CreateShaderProgram();
  if (shader_program_ == 0) {
    Shutdown();
    return false;
  }

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
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != 0) {
    image_initialized_ = true;
  }

  return true;
}

void Renderer::Shutdown() {
  for (unsigned int texture : textures_) {
    glDeleteTextures(1, &texture);
  }
  textures_.clear();

  if (index_buffer_ != 0) {
    glDeleteBuffers(1, &index_buffer_);
    index_buffer_ = 0;
  }
  if (vertex_buffer_ != 0) {
    glDeleteBuffers(1, &vertex_buffer_);
    vertex_buffer_ = 0;
  }
  if (vertex_array_ != 0) {
    glDeleteVertexArrays(1, &vertex_array_);
    vertex_array_ = 0;
  }
  if (shader_program_ != 0) {
    glDeleteProgram(shader_program_);
    shader_program_ = 0;
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

void Renderer::BeginFrame() {

  glClearColor(clear_r_/255.0f, clear_g_/255.0f, clear_b_/255.0f, clear_a_/255.0f);
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

}  // namespace engine
