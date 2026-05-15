#include "QuestionBlock.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

#include "GameConstants.hpp"

QuestionBlock::QuestionBlock(glm::vec2 position, const std::string& theme)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Theme(theme) {
    SetSprite("block/" + m_Theme + "/question_block/question_block-1.png");
}

BlockHitResult QuestionBlock::OnHit(Player* /*player*/) {
    if (m_IsUsed) return {}; // 已經敲出東西，不再反應

    m_IsUsed = true;
    SetSprite("block/" + m_Theme + "/used_block/used_block.png");
    LOG_INFO("QuestionBlock Hit! Spawning: {}", m_ItemType);

    BlockHitResult result;
    result.spawnItem = m_ItemType;
    return result;
}
