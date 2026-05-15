#ifndef WALL_BLOCK_HPP
#define WALL_BLOCK_HPP

#include <string>

#include "Block.hpp"

/*
 * WallBlock：堅硬的石塊 / 階梯方塊
 *
 * 永遠不會碎、不會被頂開，也沒有道具。
 * 通常用來做過關前的大階梯。
 */
class WallBlock : public Block {
public:
    explicit WallBlock(glm::vec2 position, const std::string& theme = "ground");

    Type GetType() const override { return Type::Wall; }
    bool IsSolid() const override { return true; }
};

#endif
