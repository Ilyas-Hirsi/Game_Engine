#pragma once
#include <vector>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include "Entity.h"
namespace engine {

template <class T> struct SparseSet {
    static constexpr uint32_t NONE = std::numeric_limits<uint32_t>::max();

    std::vector<uint32_t> sparse; // sprase set of entity ids
    std::vector<uint32_t> dense_entities; // dense set of entity ids
    std::vector<T>        dense;           // dense set of components

    bool contains(uint32_t e) const { return e < sparse.size() && sparse[e] != NONE; }
    T&   get(uint32_t e)            { return dense[sparse[e]]; }

    T& emplace(uint32_t e, T v) {
        if (e >= sparse.size()) sparse.resize(e + 1, NONE);
        sparse[e] = static_cast<uint32_t>(dense.size());
        dense_entities.push_back(e);
        dense.push_back(std::move(v));
        return dense.back();
    }

    void remove(uint32_t e) {
        uint32_t index = sparse[e];
        uint32_t last  = static_cast<uint32_t>(dense.size() - 1);

        dense_entities[index] = dense_entities[last];
        dense[index]          = std::move(dense[last]);
        sparse[dense_entities[index]] = index;

        dense_entities.pop_back();
        dense.pop_back();
        sparse[e] = NONE;
    }
};

template <typename... Components>
class View {
    public:
    explicit View(SparseSet<Components>&... pools) : pools_(&pools...) {}

    template <typename Func>
    void each(Func func) {
        auto* lead = std::get<0>(pools_);

        for (std::size_t i = lead->dense_entities.size(); i-- > 0; ) {
            uint32_t e = lead->dense_entities[i];
            if ((std::get<SparseSet<Components>*>(pools_)->contains(e) && ...)) {
                func(e, std::get<SparseSet<Components>*>(pools_)->get(e)...);
            }
        }
    }

    private:
        std::tuple<SparseSet<Components>*...> pools_;
};

template <typename... component_types>
class ECSRegistry {
    private:
    std::tuple<SparseSet<component_types>...> storage_;

    public:
    template <typename T>
    SparseSet<T>& pool() { return std::get<SparseSet<T>>(storage_); }

    template <typename T, typename... Args>
    T& emplace(const Entity& entity, Args&&... args) {
        return pool<T>().emplace(entity.GetId(),
                                 T{std::forward<Args>(args)...});
    }

    template <typename T>
    bool has(const Entity& entity) {
        return pool<T>().contains(entity.GetId());
    }

    template <typename T>
    T& getComponent(const Entity& entity) {
        return pool<T>().get(entity.GetId());
    }
    template <typename... Components>
    View<Components...> view() {
        return View<Components...>(pool<Components>()...);
    }
};

}  // namespace engine
