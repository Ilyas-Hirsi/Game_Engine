#pragma once

namespace engine {

class Timer {
 public:
  void Reset();
  float Tick();

  float DeltaTime() const { return delta_time_; }
  float TotalTime() const { return total_time_; }

 private:
  // Absolute timestamps use double: at the steady_clock epoch's magnitude a
  // float's ~7 digits can't resolve a frame-sized delta. delta_time_ stays
  // float since it's always small.
  double last_time_ = 0.0;
  float delta_time_ = 0.0f;
  double total_time_ = 0.0;
  bool first_tick_ = true;
};

}  // namespace engine
