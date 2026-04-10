#include "LevelLoader.h"

#include <fstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace {
using json = nlohmann::json;

template <typename T>
T ReadRequired(const json& object, const char* key) {
    if (!object.contains(key)) {
        throw std::runtime_error(std::string("Missing required field: ") + key);
    }
    return object.at(key).get<T>();
}

Rect ParseRect(const json& object) {
    return {
        ReadRequired<float>(object, "x"),
        ReadRequired<float>(object, "y"),
        ReadRequired<float>(object, "w"),
        ReadRequired<float>(object, "h"),
    };
}

LevelObject ParseObject(const json& object) {
    LevelObject levelObject;
    levelObject.id = ReadRequired<std::string>(object, "id");
    levelObject.type = LevelLoader::ParseObjectType(ReadRequired<std::string>(object, "type"));
    levelObject.x = ReadRequired<float>(object, "x");
    levelObject.y = ReadRequired<float>(object, "y");
    levelObject.width = ReadRequired<float>(object, "width");
    levelObject.height = ReadRequired<float>(object, "height");
    levelObject.visible = ReadRequired<bool>(object, "visible");
    levelObject.solid = ReadRequired<bool>(object, "solid");
    levelObject.collider = ParseRect(object.at("collider"));

    if (object.contains("render") && !object.at("render").is_null()) {
        levelObject.render = RenderData {
            ReadRequired<std::string>(object.at("render"), "spriteName"),
        };
    }

    return levelObject;
}
}

LevelData LevelLoader::LoadFromFile(const std::string& path) const {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open level file: " + path);
    }

    json root;
    try {
        input >> root;
    } catch (const json::parse_error& error) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(error.what()));
    }

    LevelData level;
    level.backgroundImagePath = ReadRequired<std::string>(root, "backgroundImage");
    level.levelWidth = ReadRequired<int>(root, "levelWidth");
    level.levelHeight = ReadRequired<int>(root, "levelHeight");

    if (!root.contains("objects") || !root.at("objects").is_array()) {
        throw std::runtime_error("Missing or invalid objects array");
    }

    for (const auto& object : root.at("objects")) {
        level.objects.push_back(ParseObject(object));
    }

    return level;
}

ObjectType LevelLoader::ParseObjectType(const std::string& type) {
    if (type == "Ground") {
        return ObjectType::Ground;
    }
    if (type == "Brick") {
        return ObjectType::Brick;
    }
    if (type == "QuestionBlock") {
        return ObjectType::QuestionBlock;
    }
    if (type == "Pipe") {
        return ObjectType::Pipe;
    }
    if (type == "Flag") {
        return ObjectType::Flag;
    }
    if (type == "Stair") {
        return ObjectType::Stair;
    }
    if (type == "SpawnPoint") {
        return ObjectType::SpawnPoint;
    }
    if (type == "EnemySpawn") {
        return ObjectType::EnemySpawn;
    }
    if (type == "Trigger") {
        return ObjectType::Trigger;
    }
    return ObjectType::Unknown;
}
