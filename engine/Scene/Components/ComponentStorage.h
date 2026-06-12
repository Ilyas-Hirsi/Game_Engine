#pragma once

#include <cstddef>
#include <vector>

namespace engine {
    template <typename T>
    class ComponentStorage {
    public:
    T& Add(const T& component) {
        components_.push_back(component);
        return components_.back();
    }

    T& Get(std::size_t index) {
        return components_[index];
    }

    const T& Get(std::size_t index) const {
        return components_[index];
    }

    std::vector<T>& All() {
        return components_;
    }

    const std::vector<T>& All() const {
        return components_;
    }

    std::size_t Size() const {
        return components_.size();
    }

    bool Empty() const {
        return components_.empty();
    }

    private:
    std::vector<T> components_;
    };
}