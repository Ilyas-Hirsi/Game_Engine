#pragma once
#include <string>
#include <vector>
#include "Components/ComponentType.h"
#include "Components/TransformComponent.h"
namespace engine {
    class Entity {
        public:
        Entity(int id, const std::string& name);
        int GetId() const;
        const std::string& GetName() const;
        void AddComponent(ComponentType type, int component);
        void RemoveComponent(ComponentType type);
        bool HasComponent(ComponentType type) const;
        int GetComponentIndex(ComponentType type) const;
        private:
        std::uint32_t id_;
        std::string name_;
        std::vector<int> components_ =
            std::vector<int>(engine::ComponentTypeCount, -1);

    };
}