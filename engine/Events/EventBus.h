#pragma once
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine {


class EventBus {
 public:
  template <typename T>
  using Handler = std::function<void(const T&)>;

  template <typename T>
  void Subscribe(Handler<T> handler) {
    Channel<T>().handlers.push_back(std::move(handler));
  }

  template <typename T>
  void Publish(const T& event) {
    Channel<T>().queue.push_back(event);
  }

  template <typename T, typename... Args>
  void Emplace(Args&&... args) {
    Channel<T>().queue.emplace_back(std::forward<Args>(args)...);
  }

  void Dispatch() {
    for (auto& entry : channels_) {
      entry.second->Flush();
    }
  }

  void Clear() {
    for (auto& entry : channels_) {
      entry.second->Clear();
    }
  }

 private:
  struct IChannel {
    virtual ~IChannel() = default;
    virtual void Flush() = 0;
    virtual void Clear() = 0;
  };

  template <typename T>
  struct TypedChannel : IChannel {
    std::vector<Handler<T>> handlers;
    std::vector<T> queue;

    void Flush() override {
      // Drain into a local copy first so a handler that publishes the same
      // event type does not grow the container we are iterating; those new
      // events are delivered on the following Dispatch() instead.
      std::vector<T> pending;
      pending.swap(queue);
      for (const T& event : pending) {
        for (auto& handler : handlers) {
          handler(event);
        }
      }
    }

    void Clear() override { queue.clear(); }
  };

  // Returns the channel for T, creating it on first use. The type_index key
  // guarantees the stored channel is exactly a TypedChannel<T>, so the cast
  // back from the type-erased IChannel is safe.
  template <typename T>
  TypedChannel<T>& Channel() {
    const std::type_index key(typeid(T));
    auto it = channels_.find(key);
    if (it == channels_.end()) {
      auto channel = std::make_unique<TypedChannel<T>>();
      TypedChannel<T>& ref = *channel;
      channels_.emplace(key, std::move(channel));
      return ref;
    }
    return static_cast<TypedChannel<T>&>(*it->second);
  }

  std::unordered_map<std::type_index, std::unique_ptr<IChannel>> channels_;
};

}  // namespace engine
