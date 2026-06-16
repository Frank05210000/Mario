#include "TreePlatformBlock.hpp"

#include <algorithm>

#include "GameConstants.hpp"

// 樹平台只保留碰撞範圍；外觀由背景圖負責，故不建立任何 sprite，也不覆寫 Draw。
TreePlatformBlock::TreePlatformBlock(glm::vec2 position, int segments)
    : Block(position, {std::max(2, segments) * TILE_SIZE, TILE_SIZE}) {}
