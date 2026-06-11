#ifndef HIDDEN_BLOCK_HPP
#define HIDDEN_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

class HiddenBlock : public Block {
public:
    HiddenBlock(glm::vec2 position, const ThemeAssets& assets);

    Type GetType() const override { return Type::Hidden; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    ThemeAssets m_Assets;
};

#endif
