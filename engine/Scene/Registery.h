#pragma once
#include <vector>
#include <tuple>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include "Entity.h"
#include "core/Assert.h"
#include "core/TaskScheduler.h"
namespace engine {

template <class T> struct SparseSet {
    static constexpr uint32_t NONE = std::numeric_limits<uint32_t>::max();

    std::vector<entity_t> sparse;
    std::vector<entity_t> dense_entities;
    std::vector<T>        dense;

    bool contains(entity_t e) const { return index_of(e) < sparse.size() && sparse[index_of(e)] != NONE; }
    T&   get(entity_t e)            { 
        ENGINE_ASSERT(contains(e), "Entity not found in SparseSet");
        return dense[sparse[index_of(e)]]; }

    // attempt to get a component, return nullptr if not found
    T* try_get(entity_t e) {
        if (!contains(e)) return nullptr;
        return &dense[sparse[index_of(e)]];
    }

    T& emplace(entity_t e, T v) {
        if (index_of(e) >= sparse.size()) sparse.resize(index_of(e) + 1, NONE);
        sparse[index_of(e)] = static_cast<uint32_t>(dense.size());
        dense_entities.push_back(e);
        dense.push_back(std::move(v));
        return dense.back();
    }

    void remove(entity_t e) {
        uint32_t index = sparse[index_of(e)];
        uint32_t last  = static_cast<uint32_t>(dense.size() - 1);

        dense_entities[index] = dense_entities[last];
        dense[index]          = std::move(dense[last]);
        sparse[index_of(dense_entities[index])] = index;

        dense_entities.pop_back();
        dense.pop_back();
        sparse[index_of(e)] = NONE;
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
            entity_t e = lead->dense_entities[i];
            if ((std::get<SparseSet<Components>*>(pools_)->contains(e) && ...)) {
                func(e, std::get<SparseSet<Components>*>(pools_)->get(e)...);
            }
        }
    }
        template <typename Func>
    void par_each(TaskScheduler& scheduler, Func func) {
        auto* lead = std::get<0>(pools_);
        const std::size_t n = lead->dense_entities.size();

        scheduler.parallel_for(0, n, [&](std::size_t i) {
            entity_t e = lead->dense_entities[i];
            if ((std::get<SparseSet<Components>*>(pools_)->contains(e) && ...)) {
                func(e, std::get<SparseSet<Components>*>(pools_)->get(e)...);
            }
        });
    }

    private:
        std::tuple<SparseSet<Components>*...> pools_;
};

template <typename... component_types>
class ECSRegistry {
    private:
    std::tuple<SparseSet<component_types>...> storage_;

    public:
    // Lets callers expand the component type list without repeating it.
    using component_list = std::tuple<component_types...>;

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
   template <typename T>
   void remove(const Entity& entity) {
    auto& p = pool<T>();
    if (p.contains(entity.GetId())) {
        p.remove(entity.GetId());
        }
    }

    void removeAll(entity_t entity_id) {
        std::apply([entity_id](SparseSet<component_types>&... pools) {
            ((pools.contains(entity_id) && (pools.remove(entity_id), true)), ...);
        }, storage_);
    }
};

}  // namespace engine
