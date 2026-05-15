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
            return MakeAssetPath("player/Mario/right/Walk1/Walk1.png");
        case Player::Form::SUPER:
            return MakeAssetPath("player/Super Mario/right/Walk1/Walk1.png");
        case Player::Form::FIRE:
            return MakeAssetPath("player/Fiery Mario/right/Walk1/Walk1.png");
    }
    return "";
}

std::string AssetCatalog::GetEnemySprite(Enemy::Type type) {
    switch (type) {
        case Enemy::Type::GOOMBA:
            return MakeAssetPath("enemy/Goomba/ground/normal/walk/walk-1.png");
        case Enemy::Type::KOOPA:
            return MakeAssetPath("enemy/Koopa/ground/normal/walk/walk-1.png");
    }
    return "";
}

std::string AssetCatalog::GetBlockSprite(Block::Type type) {
    switch (type) {
        case Block::Type::GROUND:
            return MakeAssetPath("block/ground/floor/floor.png");
        case Block::Type::BRICK:
            return MakeAssetPath("block/ground/brick/brick.png");
        case Block::Type::QUESTION:
            return MakeAssetPath("block/ground/question_block/question_block-1.png");
        case Block::Type::PIPE:
            return MakeAssetPath("image/Item/Pipe.gif");
        case Block::Type::FLAG:
            return MakeAssetPath("item/flag/ball.png");
    }
    return "";
}

std::string AssetCatalog::GetLevelObjectSprite(ObjectType type) {
    switch (type) {
        case ObjectType::Ground:
            return MakeAssetPath("block/ground/floor/floor.png");
        case ObjectType::Brick:
            return MakeAssetPath("block/ground/brick/brick.png");
        case ObjectType::QuestionBlock:
            return MakeAssetPath("block/ground/question_block/question_block-1.png");
        case ObjectType::Pipe:
            return MakeAssetPath("image/Item/Pipe.gif");
        case ObjectType::Flag:
            return MakeAssetPath("item/flag/ball.png");
        case ObjectType::Stair:
            return MakeAssetPath("block/ground/floor/floor.png");
        case ObjectType::SpawnPoint:
        case ObjectType::EnemySpawn:
        case ObjectType::Trigger:
        case ObjectType::Unknown:
            break;
    }
    return "";
}
