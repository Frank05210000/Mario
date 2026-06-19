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
    void Update(float deltaTime) override;
    BlockHitResult OnHit(Player* player) override;

private:
    void StartBounce();

    ThemeAssets m_Assets;
    int m_RemainingCoins = 10;
    float m_BaseY = 0.0f;
    float m_BounceTimer = 0.0f;
};

#endif
