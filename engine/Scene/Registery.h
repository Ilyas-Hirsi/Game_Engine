#pragma once
#include <vector>
#include <optional>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <utility>
#include "Entity.h"
namespace engine {
template <typename... component_types>
class ECSRegistry {
    private:
    std::tuple<std::vector<std::optional<component_types>>...> storage_;

    public:
    template <typename T>
    std::vector<std::optional<T>>& getComponents() {
        return std::get<std::vector<std::optional<T>>>(storage_);
    }

    template <typename T, typename... Args>
    T& emplace(const Entity& entity, Args&&... args) {
        auto& comp_vector = getComponents<T>();
        if (entity.GetId() >= comp_vector.size()) { // amortize the size increase of the vector
            comp_vector.resize(get_amortized_size(entity.GetId()));
        }
        comp_vector[entity.GetId()].emplace(std::forward<Args>(args)...);
        return *comp_vector[entity.GetId()];
    }

    template <typename T>
    bool has(const Entity& entity) {
        auto& comp_vector = getComponents<T>();
        return entity.GetId() < comp_vector.size() &&
               comp_vector[entity.GetId()].has_value();
    }

    template <typename T>
    T& getComponent(const Entity& entity) {
        return *getComponents<T>()[entity.GetId()];
    }

    static std::size_t get_amortized_size(std::uint32_t id) {
        std::size_t size = 1; // smallest power of two strictly greater than id
        while (size <= id) {
            size <<= 1;
        }
        return size;
    }
};

}
