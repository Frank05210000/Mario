#include "MultiCoinBlock.hpp"

#include <algorithm>

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

MultiCoinBlock::MultiCoinBlock(glm::vec2 position, const std::string& theme, int coinCount)
    : Block(position, {TILE_SIZE, TILE_SIZE}),
      m_Theme(theme),
      m_RemainingCoins(std::max(1, coinCount)) {
    SetSprite("block/" + m_Theme + "/brick/brick.png");
}

BlockHitResult MultiCoinBlock::OnHit(Player* /*player*/) {
    BlockHitResult result;
    if (m_RemainingCoins <= 0) return result;

    --m_RemainingCoins;
    result.spawnItem = "Coin";
    LOG_INFO("MultiCoinBlock hit. Remaining coins: {}", m_RemainingCoins);

    if (m_RemainingCoins == 0) {
        m_IsUsed = true;
        SetSprite("block/" + m_Theme + "/used_block/used_block.png");
    }

    return result;
}
