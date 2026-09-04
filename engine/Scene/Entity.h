#pragma once
#include <cstdint>
#include <vector>
#include "Components/ComponentType.h"
#include "Components/TransformComponent.h"
#include "core/Assert.h"
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
        static constexpr entity_t kFreeListEnd = INDEX_MASK;

        std::vector<entity_t> entities_;
        entity_t free_head_ = kFreeListEnd;
        std::size_t liveCount_ = 0;

        public:
        entity_t create(){
            liveCount_++;
            if (free_head_ != kFreeListEnd) {
                const entity_t idx = free_head_;
                const entity_t stored = entities_[idx];
                free_head_ = index_of(stored);
                entities_[idx] = (version_of(stored) << INDEX_BITS) | idx;
                return entities_[idx];

            }
            ENGINE_ASSERT(entities_.size() < kFreeListEnd, "entity index space exhausted");
            const entity_t e = static_cast<entity_t>(entities_.size());   // version is 0
            entities_.push_back(e);
            return e;
        }
        bool destroy(entity_t e) {
            if (!valid(e)) return false;
            entity_t idx = index_of(e);
            entities_[idx] = ((version_of(e) + 1) << INDEX_BITS) | free_head_; 
            free_head_ = idx;
            liveCount_--;
            return true;
        }
    
        bool valid(entity_t e) const {
            entity_t idx = index_of(e);
            return idx < entities_.size() && entities_[idx] == e;
        }
        std::size_t size()     const { return liveCount_; }
        std::size_t capacity() const { return entities_.size(); }
        bool slot_alive(std::size_t i) const {
          ENGINE_ASSERT(i < entities_.size(), "slot index out of range");
          return index_of(entities_[i]) == static_cast<entity_t>(i);
        }
        entity_t slot(std::size_t i) const {
          ENGINE_ASSERT(i < entities_.size(), "slot index out of range");
          return entities_[i];
        }

    };
}
