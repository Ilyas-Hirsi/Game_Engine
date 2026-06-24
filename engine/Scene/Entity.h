#pragma once
#include <cstdint>
#include "Components/ComponentType.h"
#include "Components/TransformComponent.h"
namespace engine {
    class Entity {
        public:
        explicit Entity(std::uint32_t id);
        int GetId() const;

        private:
        std::uint32_t id_;
    };
}
