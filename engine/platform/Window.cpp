#include "Window.h"

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include <algorithm>

namespace engine {

namespace {

bool TryMapKeyCode(SDL_Keycode sdl_key, KeyCode& key) {
  switch (sdl_key) {
    case SDLK_w:
      key = KeyCode::W;
      return true;
    case SDLK_a:
      key = KeyCode::A;
      return true;
    case SDLK_s:
      key = KeyCode::S;
      return true;
    case SDLK_d:
      key = KeyCode::D;
      return true;
    case SDLK_ESCAPE:
      key = KeyCode::Escape;
      return true;
    case SDLK_UP:
      key = KeyCode::Up;
      return true;
    case SDLK_DOWN:
      key = KeyCode::Down;
      return true;
    case SDLK_LEFT:
      key = KeyCode::Left;
      return true;
    case SDLK_RIGHT:
      key = KeyCode::Right;
      return true;
    case SDLK_q:
      key = KeyCode::Q;
      return true;
    case SDLK_e:
      key = KeyCode::E;
      return true;
    default:
      return false;
  }
}

}  // namespace

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

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width_, height_, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
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

void Window::PollEvents(Input& input) {
  input.BeginFrame();

  const bool imgui_active = ImGui::GetCurrentContext() != nullptr;
  // Makes sure input to UI is not passed to the game.
  const bool imgui_wants_keyboard =
      imgui_active && ImGui::GetIO().WantCaptureKeyboard;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (imgui_active) ImGui_ImplSDL2_ProcessEvent(&event);

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
        if (event.key.keysym.sym == SDLK_ESCAPE && !imgui_wants_keyboard) {
          should_close_ = true;
        }
        if (!imgui_wants_keyboard && !event.key.repeat) {
          KeyCode key;
          if (TryMapKeyCode(event.key.keysym.sym, key)) {
            input.SetKeyDown(key);
          }
        }
        break;
      case SDL_KEYUP:
        if (!imgui_wants_keyboard) {
          KeyCode key;
          if (TryMapKeyCode(event.key.keysym.sym, key)) {
            input.SetKeyUp(key);
          }
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
