#ifndef WALL_BLOCK_HPP
#define WALL_BLOCK_HPP

#include "ThemeAssets.hpp"

#include "Block.hpp"

/*
 * WallBlock：隱形的堅硬牆 / 階梯碰撞
 *
 * 永遠不會碎、不會被頂開，也沒有道具；只保留碰撞。
 * 背景圖已經畫出牆磚時，用它補上實體地形。
 */
class WallBlock : public Block {
public:
    explicit WallBlock(glm::vec2 position, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    Type GetType() const override { return Type::Wall; }
    bool IsSolid() const override { return true; }
};

#endif
