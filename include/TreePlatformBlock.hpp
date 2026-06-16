#ifndef TREE_PLATFORM_BLOCK_HPP
#define TREE_PLATFORM_BLOCK_HPP

#include "Block.hpp"

class TreePlatformBlock : public Block {
public:
    TreePlatformBlock(glm::vec2 position, int segments);

    Type GetType() const override { return Type::TreePlatform; }
    bool IsSolid() const override { return true; }
    bool IsOneWay() const override { return true; }  // 原版樹平台可從下方穿過
    // 不繪製：樹的外觀已包含在背景圖中，這裡只提供碰撞。
};

#endif
