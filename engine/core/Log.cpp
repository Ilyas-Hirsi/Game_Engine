#include "core/Log.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace engine {

namespace {

const char* LevelName(LogLevel level) {
  switch (level) {
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
  }
  return "????";
}

std::string Timestamp() {
  using Clock = std::chrono::system_clock;
  const auto now = Clock::now();
  const auto time = Clock::to_time_t(now);
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) %
      1000;

  std::ostringstream stream;
  stream << std::put_time(std::localtime(&time), "%H:%M:%S") << '.' << std::setfill('0')
         << std::setw(3) << ms.count();
  return stream.str();
}

}  // namespace

void Log(LogLevel level, std::string_view message) {
  std::ostream& out = level == LogLevel::Error ? std::cerr : std::cout;
  out << '[' << Timestamp() << "][" << LevelName(level) << "] " << message << '\n';
}

}  // namespace engine
