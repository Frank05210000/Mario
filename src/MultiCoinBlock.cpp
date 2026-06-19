#include "MultiCoinBlock.hpp"

#include <algorithm>
#include <cmath>

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

MultiCoinBlock::MultiCoinBlock(glm::vec2 position, const ThemeAssets& assets, int coinCount)
    : Block(position, {TILE_SIZE, TILE_SIZE}),
      m_Assets(assets),
      m_RemainingCoins(std::max(1, coinCount)),
      m_BaseY(position.y) {
    SetSprite(m_Assets.Sprite("block/{theme}/brick/brick.png"));
}

void MultiCoinBlock::Update(float deltaTime) {
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

void MultiCoinBlock::StartBounce() {
    constexpr float BOUNCE_DURATION = 0.18f;
    m_BounceTimer = BOUNCE_DURATION;
    m_Position.y = m_BaseY;
}

BlockHitResult MultiCoinBlock::OnHit(Player* /*player*/) {
    BlockHitResult result;
    if (m_RemainingCoins <= 0) return result;

    StartBounce();
    --m_RemainingCoins;
    result.spawnItem = "Coin";
    LOG_INFO("MultiCoinBlock hit. Remaining coins: {}", m_RemainingCoins);

    if (m_RemainingCoins == 0) {
        m_IsUsed = true;
        SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
    }

    return result;
}
