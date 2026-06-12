#ifndef BRICK_BLOCK_HPP
#define BRICK_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

class BrickBlock : public Block {
public:
    explicit BrickBlock(glm::vec2 position, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    Block::Type GetType() const override { return Type::Brick; }
    bool IsSolid() const override { return true; }
    void Update(float deltaTime) override;
    BlockHitResult OnHit(Player* player) override;

private:
    void StartBounce();

    ThemeAssets m_Assets;
    float m_BaseY = 0.0f;
    float m_BounceTimer = 0.0f;
};

#endif
