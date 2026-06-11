#include "HiddenBlock.hpp"

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

HiddenBlock::HiddenBlock(glm::vec2 position, const ThemeAssets& assets)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Assets(assets) {
    SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
    SetVisible(false);
}

BlockHitResult HiddenBlock::OnHit(Player* /*player*/) {
    BlockHitResult result;
    if (m_IsUsed) return result;

    m_IsUsed = true;
    SetVisible(true);
    result.spawnItem = m_ItemType.empty() ? "Coin" : m_ItemType;
    LOG_INFO("HiddenBlock revealed. Spawning: {}", result.spawnItem);
    return result;
}
