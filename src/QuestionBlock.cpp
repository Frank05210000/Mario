#include "QuestionBlock.hpp"
#include "Player.hpp"
#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Logger.hpp"

#include <cmath>
#include <vector>

QuestionBlock::QuestionBlock(glm::vec2 position, const ThemeAssets& assets)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Assets(assets), m_BaseY(position.y) {

    m_IdleAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            m_Assets.Sprite("block/{theme}/question_block/question_block-1.png"),
            m_Assets.Sprite("block/{theme}/question_block/question_block-2.png"),
            m_Assets.Sprite("block/{theme}/question_block/question_block-3.png"),
        },
        true, 120, true);

    SetDrawable(m_IdleAnim);
    const auto textureSize = m_IdleAnim->GetSize();
    if (textureSize.x > 0.0f && textureSize.y > 0.0f) {
        m_Transform.scale = {
            (m_Size.x * GAME_SCALE) / textureSize.x,
            (m_Size.y * GAME_SCALE) / textureSize.y,
        };
    }
    SetZIndex(1.0f);
}

void QuestionBlock::Update(float deltaTime) {
    if (m_BounceTimer <= 0.0f) return;

    constexpr float BOUNCE_DURATION = 0.18f;
    constexpr float BOUNCE_HEIGHT   = 5.0f;

    m_BounceTimer -= deltaTime;
    if (m_BounceTimer <= 0.0f) {
        m_BounceTimer = 0.0f;
        m_Position.y  = m_BaseY;
        return;
    }

    const float progress = 1.0f - (m_BounceTimer / BOUNCE_DURATION);
    m_Position.y = m_BaseY - std::sin(progress * 3.1415926f) * BOUNCE_HEIGHT;
}

void QuestionBlock::StartBounce() {
    constexpr float BOUNCE_DURATION = 0.18f;
    m_BounceTimer = BOUNCE_DURATION;
    m_Position.y  = m_BaseY;
}

BlockHitResult QuestionBlock::OnHit(Player* /*player*/) {
    if (m_IsUsed) return {}; // 已經敲出東西，不再反應

    m_IsUsed = true;
    StartBounce();
    SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
    LOG_INFO("QuestionBlock Hit! Spawning: {}", m_ItemType);

    BlockHitResult result;
    result.spawnItem = m_ItemType;
    return result;
}
