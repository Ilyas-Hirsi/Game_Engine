#pragma once
#include <string>
#include <vector>
#include "Component.h"
namespace engine {
    class Entity {
        public:
        private:
        int id_;
        std::string name_;
        std::vector<Component> components_;
        
    };
}