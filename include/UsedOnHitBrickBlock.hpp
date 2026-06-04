#ifndef USED_ON_HIT_BRICK_BLOCK_HPP
#define USED_ON_HIT_BRICK_BLOCK_HPP

#include <string>

#include "Block.hpp"

class UsedOnHitBrickBlock : public Block {
public:
    explicit UsedOnHitBrickBlock(glm::vec2 position, const std::string& theme = "ground");

    Type GetType() const override { return Type::UsedOnHitBrick; }
    bool IsSolid() const override { return true; }
    void Update(float deltaTime) override;
    BlockHitResult OnHit(Player* player) override;

private:
    void StartBounce();

    std::string m_Theme = "ground";
    float m_BaseY = 0.0f;
    float m_BounceTimer = 0.0f;
};

#endif
