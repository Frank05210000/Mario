#include "MultiCoinBlock.hpp"

#include <algorithm>

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

MultiCoinBlock::MultiCoinBlock(glm::vec2 position, const ThemeAssets& assets, int coinCount)
    : Block(position, {TILE_SIZE, TILE_SIZE}),
      m_Assets(assets),
      m_RemainingCoins(std::max(1, coinCount)) {
    SetSprite(m_Assets.Sprite("block/{theme}/brick/brick.png"));
}

BlockHitResult MultiCoinBlock::OnHit(Player* /*player*/) {
    BlockHitResult result;
    if (m_RemainingCoins <= 0) return result;

    --m_RemainingCoins;
    result.spawnItem = "Coin";
    LOG_INFO("MultiCoinBlock hit. Remaining coins: {}", m_RemainingCoins);

    if (m_RemainingCoins == 0) {
        m_IsUsed = true;
        SetSprite(m_Assets.Sprite("block/{theme}/used_block/used_block.png"));
    }

    return result;
}
