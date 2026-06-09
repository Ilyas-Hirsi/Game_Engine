#pragma once

namespace engine {

class Timer {
 public:
  void Reset();
  float Tick();

  float DeltaTime() const { return delta_time_; }
  float TotalTime() const { return total_time_; }

 private:
  float last_time_ = 0.0f;
  float delta_time_ = 0.0f;
  float total_time_ = 0.0f;
  bool first_tick_ = true;
};

}  // namespace engine
