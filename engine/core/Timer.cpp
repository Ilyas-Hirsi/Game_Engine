#include "core/Timer.h"

#include <algorithm>
#include <chrono>

namespace engine {

namespace {

constexpr float kMaxDeltaSeconds = 0.1f;

double NowSeconds() {
  using Clock = std::chrono::steady_clock;
  return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

}  // namespace

void Timer::Reset() {
  last_time_ = NowSeconds();
  delta_time_ = 0.0f;
  total_time_ = 0.0f;
  first_tick_ = true;
}

float Timer::Tick() {
  const double now = NowSeconds();

  if (first_tick_) {
    first_tick_ = false;
    last_time_ = now;
    delta_time_ = 0.0f;
    return delta_time_;
  }

  delta_time_ = static_cast<float>(now - last_time_);
  delta_time_ = std::min(delta_time_, kMaxDeltaSeconds);
  last_time_ = now;
  total_time_ += delta_time_;
  return delta_time_;
}

}  // namespace engine
