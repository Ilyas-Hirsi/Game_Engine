#pragma once
#include "Window.h"
#include "Renderer.h"
#include "../core/Timer.h"
#include "../Scene/Scene.h"
#include "../Scene/Systems/System.h"
#include "../ui/ImGuiLayer.h"
#include "../assets/AssetRegistry.h"
#include <string>
namespace engine {


    class Application {
        public:
        Application(const std::string& name, int width, int height);
        virtual ~Application();
        int Run();
        protected:
                virtual void OnStartup();
        virtual void OnShutdown();
        virtual void OnUpdate(float deltaTime);
        virtual void OnRender();
        virtual void OnImGui();
        Window& GetWindow();
        Renderer& GetRenderer();
        Timer& GetTimer();
        Scene& GetScene();
        Input& GetInput();
        AssetRegistry& GetAssets();
        bool IsPaused() const { return paused_; }
        void SetPaused(bool paused) { paused_ = paused; }
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        private:
        void InitializeEngine();
        void ShutDownEngine();
        void UpdateFpsStats(float deltaTime);
        float fps_timer_ = 0.0f;
        int frame_count_ = 0;
        int exit_code_ = 0;
        bool paused_ = false;
        Window window_;
        Renderer renderer_;
        Timer timer_;
        Input input_;
        Scene scene_;
        AssetRegistry assets_;
        ImGuiLayer imgui_layer_;

    };
}
