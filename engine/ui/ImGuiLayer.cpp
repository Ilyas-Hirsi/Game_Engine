#include "ImGuiLayer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include "../core/Log.h"
#include "../platform/Renderer.h"
#include "../platform/Window.h"

namespace engine {

ImGuiLayer::~ImGuiLayer() { Shutdown(); }

bool ImGuiLayer::Init(Window& window, Renderer& renderer) {
  if (initialized_) return true;

  SDL_Window* native_window = window.GetNativeWindow();
  SDL_GLContext gl_context = renderer.GetGLContext();
  if (native_window == nullptr || gl_context == nullptr) {
    LogError("ImGuiLayer::Init requires an initialized window and GL context.");
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // dockable editor panels

  ImGui::StyleColorsDark();

  if (!ImGui_ImplSDL2_InitForOpenGL(native_window, gl_context)) {
    LogError("ImGui SDL2 backend failed to initialize.");
    ImGui::DestroyContext();
    return false;
  }
  // The window is created with a 3.3 core profile so
  // the backend needs the matching GLSL version string.
  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    LogError("ImGui OpenGL3 backend failed to initialize.");
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  initialized_ = true;
  return true;
}

void ImGuiLayer::Shutdown() {
  if (!initialized_) return;
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  initialized_ = false;
}

void ImGuiLayer::BeginFrame() {
  if (!initialized_) return;
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
  if (!initialized_) return;
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace engine
