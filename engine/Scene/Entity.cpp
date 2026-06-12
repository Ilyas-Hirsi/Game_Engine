#include "Entity.h"

namespace engine {
    Entity::Entity(int id, const std::string& name) : id_(id), 
    name_(name) {}
    int Entity::GetId() const { return id_; }
    const std::string& Entity::GetName() const { return name_; }
    void Entity::AddComponent(ComponentType type, int component) {
        components_[static_cast<int>(type)] = component;
    }
    void Entity::RemoveComponent(ComponentType type) {
        components_[static_cast<int>(type)] = -1;
    }
    int Entity::GetComponentIndex(ComponentType type) const {
        return components_[static_cast<int>(type)];
    }
    bool Entity::HasComponent(ComponentType type) const {
        return components_[static_cast<int>(type)] != -1;
    }

}