#include "AssetCatalog.hpp"

#include <filesystem>
#include <string>

namespace {
std::string MakeAssetPath(const std::string& relativePath) {
    const auto assetRoot =
        std::filesystem::path(RESOURCE_DIR).parent_path() / "Asset";
    return (assetRoot / relativePath).string();
}
}

std::string AssetCatalog::GetPlayerSprite(Player::Form form) {
    switch (form) {
        case Player::Form::SMALL:
            return MakeAssetPath("image/Character/Mario/Mario.gif");
        case Player::Form::SUPER:
            return MakeAssetPath("image/Character/Super_Mario/Super Mario.gif");
        case Player::Form::FIRE:
            return MakeAssetPath("image/Character/Fiery_Mario/Fiery Mario.gif");
    }
    return "";
}

std::string AssetCatalog::GetEnemySprite(Enemy::Type type) {
    switch (type) {
        case Enemy::Type::GOOMBA:
            return MakeAssetPath("image/Amey/ground/Goomba.gif");
        case Enemy::Type::KOOPA:
            return MakeAssetPath("image/Amey/ground/Green Koopa Troopa.gif");
    }
    return "";
}

std::string AssetCatalog::GetBlockSprite(Block::Type type) {
    switch (type) {
        case Block::Type::GROUND:
            return MakeAssetPath("image/Scenery/Ground.gif");
        case Block::Type::BRICK:
            return MakeAssetPath("image/Item/Bricks.gif");
        case Block::Type::QUESTION:
            return MakeAssetPath("image/Item/Question Block.gif");
        case Block::Type::PIPE:
            return MakeAssetPath("image/Item/Pipe.gif");
        case Block::Type::FLAG:
            return MakeAssetPath("image/Item/Goal Flag.gif");
    }
    return "";
}
