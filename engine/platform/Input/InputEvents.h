#pragma once
#include "KeyCode.h"

namespace engine {

struct KeyPressedEvent {
  KeyCode key;
};

struct KeyReleasedEvent {
  KeyCode key;
};

struct MouseMovedEvent {
  int x;
  int y;
};

struct MouseButtonPressedEvent {
  int button;
};

struct MouseButtonReleasedEvent {
  int button;
};

}  // namespace engine
