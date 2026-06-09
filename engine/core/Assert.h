#pragma once

#include "core/Log.h"

#include <cstdlib>

namespace engine {

[[noreturn]] inline void Fatal(std::string_view message) {
  LogError(message);
  std::abort();
}

}  // namespace engine

#define ENGINE_ASSERT(condition, message) \
  do {                                  \
    if (!(condition)) {                 \
      engine::Fatal(message);           \
    }                                   \
  } while (0)
