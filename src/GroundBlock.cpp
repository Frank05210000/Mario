#include "GroundBlock.hpp"

#include "GameConstants.hpp"

GroundBlock::GroundBlock(glm::vec2 position, glm::vec2 size)
    : Block(position, size) {
    // 隱形碰撞盒：不需要設定圖片 (SetSprite)，Renderer 自然不會畫它
    SetVisible(false);
}
