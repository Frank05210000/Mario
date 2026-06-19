#ifndef MULTI_COIN_BLOCK_HPP
#define MULTI_COIN_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

class MultiCoinBlock : public Block {
public:
    MultiCoinBlock(glm::vec2 position, const ThemeAssets& assets, int coinCount = 10);

    Type GetType() const override { return Type::MultiCoin; }
    bool IsSolid() const override { return true; }
    bool BumpsContentsAbove() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    ThemeAssets m_Assets;
    int m_RemainingCoins = 10;
};

#endif
