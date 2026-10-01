#pragma once

#include <string>

#include "MeshData.h"

namespace engine {

// Loads a Wavefront OBJ into the engine vertex layout
MeshData LoadObj(const std::string& path);

}  // namespace engine
