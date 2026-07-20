#include "Application.h"

namespace engine {

Application::Application(const std::string& name, int width, int height)
    : window_(name, width, height), renderer_(), timer_() {
    InitializeEngine();
}

Application::~Application() {
    ShutDownEngine();
}

void Application::InitializeEngine() {
    renderer_.Initialize(window_);
}

void Application::ShutDownEngine() {
    renderer_.Shutdown();
}

int Application::Run() {
    OnStartup();
    // main loop
    while (!window_.ShouldClose()) {
        const float delta_time = timer_.Tick();
        window_.PollEvents(input_);
        OnUpdate(delta_time);
        renderer_.BeginFrame();
        OnRender();
        renderer_.EndFrame();
    }
    return exit_code_;
}

void Application::OnUpdate(float delta_time) {
    system_.HandleInput(scene_,input_, delta_time);
    scene_.Update(delta_time);
}

Renderer& Application::GetRenderer() { return renderer_; }
}