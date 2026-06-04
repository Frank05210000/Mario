#include "UsedOnHitBrickBlock.hpp"

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

#include <cmath>

UsedOnHitBrickBlock::UsedOnHitBrickBlock(glm::vec2 position, const std::string& theme)
    : Block(position, {TILE_SIZE, TILE_SIZE}), m_Theme(theme), m_BaseY(position.y) {
    SetSprite("block/" + m_Theme + "/brick/brick.png");
}

void UsedOnHitBrickBlock::Update(float deltaTime) {
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

void UsedOnHitBrickBlock::StartBounce() {
    constexpr float BOUNCE_DURATION = 0.18f;
    m_BounceTimer = BOUNCE_DURATION;
    m_Position.y = m_BaseY;
}

BlockHitResult UsedOnHitBrickBlock::OnHit(Player*) {
    BlockHitResult result;
    if (m_IsUsed) return result;

    m_IsUsed = true;
    StartBounce();
    SetSprite("block/" + m_Theme + "/used_block/used_block.png");
    if (m_ItemType != "None" && !m_ItemType.empty()) {
        result.spawnItem = m_ItemType;
    }
    LOG_INFO("UsedOnHitBrickBlock hit. Spawning: {}", result.spawnItem);
    return result;
}
