#ifndef LEVEL_DATA_HPP
#define LEVEL_DATA_HPP

#include <string>
#include <vector>

#include <glm/vec2.hpp>

/*
 * ObjectData：從 JSON 讀出的單一物件原始資料
 *
 * 這是純資料容器（struct），不含任何遊戲邏輯。
 * 讀完 JSON 之後，GameManager 依照 type 建立對應的 Block/Enemy。
 * 建完遊戲物件後，ObjectData 本身就功成身退。
 */
struct ObjectData {
    std::string type;       // "Ground" / "Brick" / "QuestionBlock" / "Pipe" / "Flag" / "Wall" / "EnemySpawn"
    std::string enemyType;  // 只有 type=="EnemySpawn" 才有值："Goomba" / "Koopa"
    std::string itemType;   // 夾帶的道具："None", "Coin", "PowerUp", "Mushroom", "Star", "CoinMulti"
    std::string variant = "green"; // Koopa/Paratroopa: "green" / "red"
    std::string flightMode = "hop"; // Paratroopa: "hop" / "verticalPatrol"
    std::string targetLevel; // 可進入水管的目標關卡，例如 "1-1_underground"
    std::string exitToLevel; // 地下出口水管可回到的關卡，例如 "1-1"
    glm::vec2 targetSpawn = {0.0f, 0.0f}; // 可選的傳送後出生點
    bool hasTargetSpawn = false;
    float x      = 0.0f;
    float y      = 0.0f;
    float width  = 16.0f;  // 預設一格 tile（16px）
    float height = 16.0f;
    std::string opening = "up"; // 開口方向: "up", "down", "left", "right"
    bool enterable = false; // 水管是否可進入
    std::string moveAxis = "horizontal"; // MovingPlatform: "horizontal" / "vertical"
    std::string moveMode = "oscillate";  // MovingPlatform: "oscillate" / "verticalWrap" / "horizontalOscillate"
    float moveDistance = 0.0f;           // MovingPlatform: 最大移動距離
    float moveSpeed = 0.0f;              // MovingPlatform: 速度 px/s
    int segments = 3;                    // TreePlatform: 平台長度（幾個 16px tile）
    int coinCount = 10;                  // MultiCoinBlock: 可敲出的金幣數
};

/*
 * LevelData：整個關卡的資料容器
 *
 * 由 LevelLoader::Load() 解析 JSON 後回傳。
 * GameManager 依此建立場景。
 */
struct LevelData {
    std::string backgroundImagePath;  // MakeAssetPath() 接受的相對路徑
    std::string theme = "ground";     // block tile 主題: "ground" / "underground"
    int levelWidth  = 0;              // 地圖總寬（世界像素）
    int levelHeight = 0;              // 地圖總高（世界像素）

    glm::vec2 playerSpawn = {64.0f, 300.0f}; // 玩家出生點 (預設值)

    std::vector<ObjectData> objects;  // 所有物件（方塊、出生點、觸發器...）
};

#endif
