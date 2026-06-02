#include "LevelLoader.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

LevelData LevelLoader::Load(const std::string& jsonPath) {
    // 1. 開檔
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        throw std::runtime_error("LevelLoader: 無法開啟檔案: " + jsonPath);
    }

    nlohmann::json j;
    file >> j;

    LevelData data;

    // 2. 讀背景圖路徑
    //    JSON 格式：{ "backgroundImage": "Asset/image/stage/1-1.png" }
    //    存入 LevelData 時去掉 "Asset/" 前綴，讓 MakeAssetPath() 直接接受
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
    data.theme       = j.value("theme", "ground");

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
            od.moveDistance = obj.value("moveDistance", 0.0f);
            od.moveSpeed = obj.value("moveSpeed", 0.0f);
            od.segments = obj.value("segments", 3);
            od.coinCount = obj.value("coinCount", 10);
            if (obj.contains("targetSpawn")) {
                od.targetSpawn.x = obj["targetSpawn"].value("x", 0.0f);
                od.targetSpawn.y = obj["targetSpawn"].value("y", 0.0f);
                od.hasTargetSpawn = true;
            }
            data.objects.push_back(od);
        }
    }

    return data;
}
