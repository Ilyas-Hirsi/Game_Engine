#include "Application.h"

#include <algorithm>
#include <string>

#include "../core/Log.h"

namespace engine {

Application::Application(const std::string& name, int width, int height)
    : window_(name, width, height), renderer_(), timer_(), assets_(renderer_) {
    InitializeEngine();
}

Application::~Application() {
    ShutDownEngine();
}

void Application::InitializeEngine() {
    if (!renderer_.Initialize(window_)) {
        LogError("Failed to initialize renderer.");
        exit_code_ = 1;
        return;
    }
    // Needs GL context, so must follow renderer
    imgui_layer_.Init(window_, renderer_);
}

void Application::ShutDownEngine() {
    imgui_layer_.Shutdown();
    renderer_.Shutdown();
}

int Application::Run() {
    if (exit_code_ != 0) return exit_code_;

    OnStartup();
    timer_.Reset();

    float accumulator = 0.0f;
    const float fixed_dt = scene_.GetPhysicsSettings().fixed_dt;

    while (!window_.ShouldClose()) {
        const float delta_time = timer_.Tick();
        // Clamp so a stall can't break fixed update.
        const float frame_dt = std::min(delta_time, 0.25f);
        accumulator += frame_dt;

        window_.PollEvents(input_);
        OnUpdate(frame_dt);

        while (accumulator >= fixed_dt) {
            scene_.FixedUpdate(fixed_dt);
            accumulator -= fixed_dt;
        }
        const float alpha = fixed_dt > 0.0f ? accumulator / fixed_dt : 0.0f;

        renderer_.BeginFrame();
        scene_.Render(renderer_, alpha);
        OnRender();

        // UI last so it overlays the scene
        imgui_layer_.BeginFrame();
        OnImGui();
        imgui_layer_.EndFrame();

        renderer_.EndFrame();

        UpdateFpsStats(frame_dt);
    }

    OnShutdown();
    return exit_code_;
}

void Application::OnUpdate(float delta_time) {
    scene_.HandleInput(input_, delta_time);
    scene_.Update(delta_time);
}

void Application::OnStartup() {}
void Application::OnShutdown() {}
void Application::OnRender() {}
void Application::OnImGui() {}

void Application::UpdateFpsStats(float delta_time) {
    fps_timer_ += delta_time;
    ++frame_count_;
    if (fps_timer_ < 1.0f) return;

    const int fps = static_cast<int>(frame_count_ / fps_timer_);
    LogInfo("FPS: " + std::to_string(fps) + "  frame: " +
            std::to_string(1000.0f * fps_timer_ / frame_count_) + " ms");
    fps_timer_ = 0.0f;
    frame_count_ = 0;
}

Window& Application::GetWindow() { return window_; }
Renderer& Application::GetRenderer() { return renderer_; }
Timer& Application::GetTimer() { return timer_; }
Scene& Application::GetScene() { return scene_; }
Input& Application::GetInput() { return input_; }
AssetRegistry& Application::GetAssets() { return assets_; }
}
