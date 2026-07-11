#pragma once
#include <cstdint>
#include <vector>
#include "Components/ComponentType.h"
#include "Components/TransformComponent.h"
using entity_t = std::uint32_t; // define type entity_t to give it constant expressions
constexpr entity_t INDEX_BITS = 20;
constexpr entity_t INDEX_MASK = (1u << INDEX_BITS) - 1; // return lower 20 bits of id 
constexpr entity_t index_of(entity_t e) {return e & INDEX_MASK;}
constexpr entity_t version_of(entity_t e) {return e >> INDEX_BITS;}
namespace engine {
    class Entity {
        public:
        explicit Entity(std::uint32_t id) : id_(id) {}
        entity_t GetId() const { return id_; }
        private:
        std::uint32_t id_;
    };

    class EntityStorage {
        std::vector<entity_t> entities_;
        std::vector<entity_t> free_list_;

        public:
        entity_t create(){
            if (!free_list_.empty()) {
                entity_t idx = free_list_.back();
                free_list_.pop_back();
                return entities_[idx];
            }
            entity_t e = static_cast<entity_t>(entities_.size());
            entities_.push_back(e);
            return e;
        }
        bool destroy(entity_t e) {
            if (!valid(e)) return false;
            entity_t idx = index_of(e);
            entity_t new_version = version_of(e) + 1;
            entities_[idx] = (new_version << INDEX_BITS) | idx; // update version bits
            free_list_.push_back(idx);
            return true;
        }
    
        bool valid(entity_t e) const {
            entity_t idx = index_of(e);
            return idx < entities_.size() && entities_[idx] == e;
        }

    };
}
