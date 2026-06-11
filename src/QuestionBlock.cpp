#include "QuestionBlock.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

#include "GameConstants.hpp"

QuestionBlock::QuestionBlock(glm::vec2 position, const ThemeAssets& assets)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Assets(assets) {
    SetSprite(m_Assets.Sprite("block/{theme}/question_block/question_block-1.png"));
}

BlockHitResult QuestionBlock::OnHit(Player* /*player*/) {
    if (m_IsUsed) return {}; // 已經敲出東西，不再反應

    m_IsUsed = true;
    SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
    LOG_INFO("QuestionBlock Hit! Spawning: {}", m_ItemType);

    BlockHitResult result;
    result.spawnItem = m_ItemType;
    return result;
}
