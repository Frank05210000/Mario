#include "BrickBlock.hpp"
#include "Player.hpp"
#include "Util/Logger.hpp"

#include <cmath>

#include "GameConstants.hpp"

BrickBlock::BrickBlock(glm::vec2 position, const ThemeAssets& assets)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Assets(assets), m_BaseY(position.y) {
    SetSprite(m_Assets.Sprite("block/{theme}/brick/brick.png"));
}

void BrickBlock::Update(float deltaTime) {
    if (m_BounceTimer <= 0.0f) return;

    constexpr float BOUNCE_DURATION = 0.18f;
    constexpr float BOUNCE_HEIGHT = 5.0f;

    m_BounceTimer -= deltaTime;
    if (m_BounceTimer <= 0.0f) {
        m_BounceTimer = 0.0f;
        m_Position.y = m_BaseY;
        return;
    }

    const float progress = 1.0f - (m_BounceTimer / BOUNCE_DURATION);
    m_Position.y = m_BaseY - std::sin(progress * 3.1415926f) * BOUNCE_HEIGHT;
}

void BrickBlock::StartBounce() {
    constexpr float BOUNCE_DURATION = 0.18f;
    m_BounceTimer = BOUNCE_DURATION;
    m_Position.y = m_BaseY;
}

BlockHitResult BrickBlock::OnHit(Player* player) {
    BlockHitResult result;

    if (m_ItemType != "None" && !m_IsUsed) {
        // 如果磚塊裡有藏金幣或是道具，行為就相當於問號方塊
        m_IsUsed = true;
        StartBounce();
        SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
        LOG_INFO("BrickBlock Hit! Spawning hidden item: {}", m_ItemType);
        result.spawnItem = m_ItemType;
        return result;
    }

    if (m_IsUsed) return result; // 已經空了

    if (player && player->GetForm() == Player::Form::SMALL) {
        StartBounce();
        LOG_INFO("BrickBlock Hit by Small Mario! Bounce only.");
        return result;
    }
    
    LOG_INFO("BrickBlock Hit! Destroyed!");
    result.isDestroyed = true;
    return result;
}
