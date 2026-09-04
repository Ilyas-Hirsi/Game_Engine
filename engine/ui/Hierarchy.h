#pragma once
#include <vector>
#include "../Scene/Entity.h"


namespace engine {
    class Scene;

    void DrawHierarchy(Scene& scene, entity_t& selected, std::vector<entity_t>& scratch);
    
}
