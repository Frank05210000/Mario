#ifndef MULTI_COIN_BLOCK_HPP
#define MULTI_COIN_BLOCK_HPP

#include <string>

#include "Block.hpp"

class MultiCoinBlock : public Block {
public:
    MultiCoinBlock(glm::vec2 position, const std::string& theme, int coinCount = 10);

    Type GetType() const override { return Type::MultiCoin; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    std::string m_Theme = "ground";
    int m_RemainingCoins = 10;
};

#endif
