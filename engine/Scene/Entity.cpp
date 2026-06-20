#include "Entity.h"

namespace engine {
    Entity::Entity(int id, const std::string& name) : id_(id), 
    name_(name) {}
    int Entity::GetId() const { return id_; }
    

}

