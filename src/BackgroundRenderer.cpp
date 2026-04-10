#include "BackgroundRenderer.h"

void BackgroundRenderer::Render(const LevelData& level, IRenderer& renderer) const {
    renderer.DrawImage(
        level.backgroundImagePath,
        0.0F,
        0.0F,
        static_cast<float>(level.levelWidth),
        static_cast<float>(level.levelHeight));
}
