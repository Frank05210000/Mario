#include "HiddenBlock.hpp"

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

HiddenBlock::HiddenBlock(glm::vec2 position, const std::string& theme)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Theme(theme) {
    SetSprite("block/" + m_Theme + "/used_block/used_block.png");
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
