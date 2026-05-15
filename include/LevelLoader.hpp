#ifndef LEVEL_LOADER_HPP
#define LEVEL_LOADER_HPP

#include <string>

#include "LevelData.hpp"

/*
 * LevelLoader：關卡 JSON 解析工具
 *
 * 純工具類，不繼承任何人，不允許建立實例。
 * 只有一個靜態函式：讀 JSON → 回傳 LevelData。
 *
 * OOP 關係：
 *   - 不繼承（工具類，不是遊戲物件）
 *   - GameManager「使用」LevelLoader（依賴關係）
 *
 * 使用方式：
 *   LevelData level = LevelLoader::Load("/path/to/1-1.json");
 */
class LevelLoader {
public:
    // jsonPath：JSON 檔案的完整絕對路徑
    // 成功：回傳填好的 LevelData
    // 失敗：拋出 std::runtime_error
    static LevelData Load(const std::string& jsonPath);

private:
    LevelLoader() = delete;  // 禁止建立實例，只能用靜態函式
};

#endif
