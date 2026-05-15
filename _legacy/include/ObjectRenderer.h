#pragma once

#include "IRenderer.h"
#include "LevelTypes.h"

class ObjectRenderer {
public:
    void Render(const LevelData& level, IRenderer& renderer) const;
};
