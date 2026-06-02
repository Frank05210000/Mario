#ifndef BRICK_BLOCK_HPP
#define BRICK_BLOCK_HPP

#include <string>

#include "Block.hpp"

class BrickBlock : public Block {
public:
    explicit BrickBlock(glm::vec2 position, const std::string& theme = "ground");

    Block::Type GetType() const override { return Type::Brick; }
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
