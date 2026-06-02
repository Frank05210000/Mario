#include "TreePlatformBlock.hpp"

#include <algorithm>

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

TreePlatformBlock::TreePlatformBlock(glm::vec2 position, int segments)
    : Block(position, {std::max(2, segments) * TILE_SIZE, TILE_SIZE}) {
    const int count = std::max(2, segments);

    auto leftImg = std::make_shared<Util::Image>(MakeAssetPath("block/tree/tree_top_left.png"));
    auto midImg = std::make_shared<Util::Image>(MakeAssetPath("block/tree/tree_top_mid.png"));
    auto rightImg = std::make_shared<Util::Image>(MakeAssetPath("block/tree/tree_top_right.png"));
    auto chunkImg = std::make_shared<Util::Image>(MakeAssetPath("block/tree/tree_chunk.png"));

    for (int i = 0; i < count; ++i) {
        auto image = midImg;
        if (i == 0) image = leftImg;
        else if (i == count - 1) image = rightImg;

        auto obj = std::make_shared<Util::GameObject>(image, 1.0f);
        obj->m_Transform.scale = {GAME_SCALE, GAME_SCALE};
        AddChild(obj);
        m_Segments.push_back({obj, {i * TILE_SIZE, 0.0f}});
    }

    auto trunk = std::make_shared<Util::GameObject>(chunkImg, 0.9f);
    trunk->m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    AddChild(trunk);
    m_Segments.push_back({trunk, {((count / 2) * TILE_SIZE), TILE_SIZE}});
}

void TreePlatformBlock::Draw(const Camera& camera) {
    for (auto& segment : m_Segments) {
        const glm::vec2 pos = m_Position + segment.offset;
        const glm::vec2 centerPos = {
            pos.x + TILE_SIZE * 0.5f,
            pos.y + TILE_SIZE * 0.5f,
        };
        segment.obj->m_Transform.translation = camera.WorldToScreen(centerPos);
    }
}
