#pragma once

#include "IRenderer.h"
#include "LevelTypes.h"

class BackgroundRenderer {
public:
    void Render(const LevelData& level, IRenderer& renderer) const;
};
