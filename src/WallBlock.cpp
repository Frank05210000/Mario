#include "WallBlock.hpp"

#include "GameConstants.hpp"

WallBlock::WallBlock(glm::vec2 position, const ThemeAssets& assets)
    : Block(position, {TILE_SIZE, TILE_SIZE}) {
    (void)assets;
}
