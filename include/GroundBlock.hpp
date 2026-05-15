#ifndef GROUND_BLOCK_HPP
#define GROUND_BLOCK_HPP

#include "Block.hpp"

class GroundBlock : public Block {
public:
    explicit GroundBlock(glm::vec2 position, glm::vec2 size);

    Type GetType() const override { return Type::Ground; }
    bool IsSolid() const override { return true; }
};

#endif
