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
        
        private:
        std::uint32_t id_;
        std::string name_;


    };
}