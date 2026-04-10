#pragma once

#include <optional>
#include <string>
#include <vector>

struct Rect {
    float x;
    float y;
    float w;
    float h;
};

enum class ObjectType {
    Ground,
    Brick,
    QuestionBlock,
    Pipe,
    Flag,
    Stair,
    SpawnPoint,
    EnemySpawn,
    Trigger,
    Unknown
};

struct RenderData {
    std::string spriteName;
};

struct LevelObject {
    std::string id;
    ObjectType type = ObjectType::Unknown;
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
    bool visible = true;
    bool solid = false;
    Rect collider {0.0F, 0.0F, 0.0F, 0.0F};
    std::optional<RenderData> render;
};

struct LevelData {
    std::string backgroundImagePath;
    int levelWidth = 0;
    int levelHeight = 0;
    std::vector<LevelObject> objects;
};
