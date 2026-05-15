#ifndef BRICK_BLOCK_HPP
#define BRICK_BLOCK_HPP

#include <string>

#include "Block.hpp"

class BrickBlock : public Block {
public:
    explicit BrickBlock(glm::vec2 position, const std::string& theme = "ground");

    Block::Type GetType() const override { return Type::Brick; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    std::string m_Theme = "ground";
};

#endif
