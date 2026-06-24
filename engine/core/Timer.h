#pragma once

namespace engine {

class Timer {
 public:
  void Reset();
  float Tick();

  float DeltaTime() const { return delta_time_; }
  float TotalTime() const { return total_time_; }

 private:
  // Absolute timestamps are kept in double: the steady_clock epoch is a very
  // large value (seconds since ~boot), and a 32-bit float's ~7 significant
  // digits cannot resolve a frame-sized delta at that magnitude. delta_time_
  // stays float because it is always small.
  double last_time_ = 0.0;
  float delta_time_ = 0.0f;
  double total_time_ = 0.0;
  bool first_tick_ = true;
};

}  // namespace engine
