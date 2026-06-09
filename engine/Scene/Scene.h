#pragma once
#include <vector>
#include "Entity.h"
#include "../platform/Renderer.h"
namespace engine {

    class Scene {
        public:
        Scene();
        virtual ~Scene();
        void Update(float deltaTime);
        void Render(Renderer& renderer);
        protected:
        virtual void OnUpdate(float deltaTime);
        virtual void OnRender(Renderer& renderer);

        private:
        std::vector<Entity> entities_;

    };
}