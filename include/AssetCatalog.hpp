#ifndef ASSET_CATALOG_HPP
#define ASSET_CATALOG_HPP

#include <string>

#include "Block.hpp"
#include "Enemy.hpp"
#include "LevelTypes.h"
#include "Player.hpp"

class AssetCatalog {
public:
    static std::string GetPlayerSprite(Player::Form form);
    static std::string GetEnemySprite(Enemy::Type type);
    static std::string GetBlockSprite(Block::Type type);
    static std::string GetLevelObjectSprite(ObjectType type);
};

#endif
