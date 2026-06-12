#include "LevelLoader.hpp"

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

LevelData LevelLoader::Load(const std::string& jsonPath) {
    LOG_INFO("LevelLoader loading: {}", jsonPath);

    // 1. 開檔
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        LOG_ERROR("LevelLoader failed to open file: {}", jsonPath);
        throw std::runtime_error("LevelLoader: 無法開啟檔案: " + jsonPath);
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::exception& e) {
        LOG_ERROR("LevelLoader JSON parse error: file='{}' message='{}'", jsonPath, e.what());
        throw;
    }

    LevelData data;

    // 2. 讀背景圖路徑
    //    JSON 格式：{ "backgroundImage": "level_image/1-1/ground.png" }
    //    路徑相對於 Resources/Asset/；舊的 "Asset/..." 前綴仍相容。
    std::string rawPath = j.value("backgroundImage", "");
    const std::string prefix = "Asset/";
    if (rawPath.size() >= prefix.size() &&
        rawPath.substr(0, prefix.size()) == prefix) {
        data.backgroundImagePath = rawPath.substr(prefix.size());
    } else {
        data.backgroundImagePath = rawPath;
    }

    data.levelWidth  = j.value("levelWidth",  0);
    data.levelHeight = j.value("levelHeight", 0);
    // 主題從檔名推導，保持「檔名即真相」：含 _underground → Underground，其餘 → Ground。
    data.theme = (jsonPath.find("_underground") != std::string::npos)
                     ? Theme::Underground : Theme::Ground;

    // 解析玩家出生點
    if (j.contains("playerSpawn")) {
        data.playerSpawn.x = j["playerSpawn"].value("x", 64.0f);
        data.playerSpawn.y = j["playerSpawn"].value("y", 300.0f);
    }

    // 3. 讀物件清單
    if (j.contains("objects") && j["objects"].is_array()) {
        for (const auto& obj : j["objects"]) {
            ObjectData od;
            od.type      = obj.value("type",      "");
            if (od.type == "Pipe") {
                od.type = "EnterablePipe";
            }
            od.enemyType = obj.value("enemyType", "");
            od.itemType  = obj.value("itemType",  "");
            od.variant = obj.value("variant", "green");
            od.flightMode = obj.value("flightMode", "hop");
            od.targetLevel = obj.value("targetLevel", "");
            od.exitToLevel = obj.value("exitToLevel", "");
            od.x         = obj.value("x",         0.0f);
            od.y         = obj.value("y",         0.0f);
            od.width     = obj.value("width",     16.0f);
            od.height    = obj.value("height",    16.0f);
            od.opening   = obj.value("opening",   "up");
            od.enterable = obj.value("enterable", false);
            od.moveAxis = obj.value("moveAxis", "horizontal");
            od.moveMode = obj.value("moveMode", "oscillate");
            od.startDirection = obj.value("startDirection", "");
            if (od.startDirection.empty()) {
                od.startDirection = (od.moveAxis == "vertical") ? "down" : "right";
            }
            od.moveSpeed = obj.value("moveSpeed", 0.0f);
            if (od.type == "EnterablePipe") {
                if (obj.contains("segments")) {
                    od.segments = std::max(1, obj.value("segments", 2));
                } else if ((od.opening == "left" || od.opening == "right") && obj.contains("width")) {
                    od.segments = std::max(1, static_cast<int>(std::round(od.width / TILE_SIZE)));
                } else if (obj.contains("height")) {
                    od.segments = std::max(1, static_cast<int>(std::round(od.height / TILE_SIZE)));
                } else {
                    od.segments = 2;
                }
            } else {
                od.segments = obj.value("segments", 3);
            }
            if (od.type == "MovingPlatform") {
                if (!obj.contains("segments") && obj.contains("width")) {
                    od.segments = std::max(1, static_cast<int>(std::round(od.width / TILE_SIZE)));
                }
                if (obj.contains("moveTiles")) {
                    od.moveTiles = std::max(0, obj.value("moveTiles", 0));
                    od.moveDistance = static_cast<float>(od.moveTiles) * TILE_SIZE;
                } else {
                    od.moveDistance = obj.value("moveDistance", 0.0f);
                    od.moveTiles = std::max(0, static_cast<int>(std::round(od.moveDistance / TILE_SIZE)));
                }
            } else {
                od.moveDistance = obj.value("moveDistance", 0.0f);
            }
            od.coinCount = obj.value("coinCount", 10);
            if (obj.contains("targetSpawn")) {
                od.targetSpawn.x = obj["targetSpawn"].value("x", 0.0f);
                od.targetSpawn.y = obj["targetSpawn"].value("y", 0.0f);
                od.hasTargetSpawn = true;
            }
            data.objects.push_back(od);
        }
    }

    // 解析中繼點清單（可選，缺省為空陣列）
    // 格式：{ "checkpoints": [{"x": 2000, "y": 192}] }
    if (j.contains("checkpoints") && j["checkpoints"].is_array()) {
        for (const auto& cp : j["checkpoints"]) {
            glm::vec2 point;
            point.x = cp.value("x", 0.0f);
            point.y = cp.value("y", 0.0f);
            data.checkpoints.push_back(point);
        }
    }

    LOG_INFO("LevelLoader loaded: file='{}' background='{}' theme='{}' size=({}, {}) objects={} checkpoints={}",
             jsonPath,
             data.backgroundImagePath,
             data.theme == Theme::Underground ? "underground" : "ground",
             data.levelWidth,
             data.levelHeight,
             data.objects.size(),
             data.checkpoints.size());

    return data;
}
