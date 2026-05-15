#include "ObjectRenderer.h"

void ObjectRenderer::Render(const LevelData& level, IRenderer& renderer) const {
    for (const auto& object : level.objects) {
        if (!object.visible || !object.render.has_value()) {
            continue;
        }

        renderer.DrawSprite(
            object.render->spriteName,
            object.x,
            object.y,
            object.width,
            object.height);
    }
}
