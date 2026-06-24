#include "Entity.h"

namespace engine {
    Entity::Entity(std::uint32_t id) : id_(id) {}
    int Entity::GetId() const { return id_; }
}
