#ifndef HIDDEN_BLOCK_HPP
#define HIDDEN_BLOCK_HPP

#include <string>

#include "Block.hpp"

class HiddenBlock : public Block {
public:
    HiddenBlock(glm::vec2 position, const std::string& theme);

    Type GetType() const override { return Type::Hidden; }
    bool IsSolid() const override { return true; }
    BlockHitResult OnHit(Player* player) override;

private:
    std::string m_Theme = "ground";
};

#endif
