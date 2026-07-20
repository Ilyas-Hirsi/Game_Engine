#pragma once
#include "Window.h"
#include "Renderer.h"
#include "../core/Timer.h"
#include "../Scene/Scene.h"
#include "../Scene/Systems/Systems.h"
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
        Window& GetWindow();
        Renderer& GetRenderer();
        Timer& GetTimer();
        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        private:
        void InitializeEngine();
        void ShutDownEngine();
        void UpdateFpsStats(float deltaTime);
        float fps_timer_ = 0.0f;
        int frame_count_ = 0;
        int exit_code_ = 0;
        Window window_;
        Renderer renderer_;
        Timer timer_;
        Input input_;
        System system_;
        Scene scene_;
        // add input later

    };
}