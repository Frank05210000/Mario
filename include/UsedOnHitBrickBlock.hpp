#ifndef USED_ON_HIT_BRICK_BLOCK_HPP
#define USED_ON_HIT_BRICK_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

class UsedOnHitBrickBlock : public Block {
public:
    explicit UsedOnHitBrickBlock(glm::vec2 position, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    Type GetType() const override { return Type::UsedOnHitBrick; }
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
