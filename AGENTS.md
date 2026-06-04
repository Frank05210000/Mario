# OOPL_Mario Agent Notes

這份頂層文件只保留全 repo 都需要先知道的事項。進入特定目錄工作前，請讀該目錄自己的 `AGENTS.md`。

## 專案概況

- 類型：C++17 / CMake / PTSD framework 的 2D Mario-like 遊戲。
- 主程式：`src/main.cpp` 建立 `Core::Context` 與 `App`，依序跑 `Start -> Update -> End`。
- 核心遊戲邏輯集中在 `GameManager`，主要實作位於 `src/`，public headers 位於 `include/`。
- runtime 關卡資料從 `Resources/data/*.json` 載入。
- 資產根目錄是 `Resources/Asset/`，程式用 `MakeAssetPath()` 將相對路徑接到 `RESOURCE_DIR/Asset`。
- 內嵌 framework 在 `PTSD/`；除非是 framework 問題，通常不要改。
- 舊版程式在 `_legacy/`；可作參考，但目前 build 使用 `src/` 與 `include/`。

## 建置與執行

Debug build 是必要條件，因為 `CMakeLists.txt` 只在 Debug 定義可用的 `RESOURCE_DIR`，Release 會直接 fatal。

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
./build/OOPL_Mario
```

目前已驗證 `cmake --build build` 可成功完成。

如果新增 `.cpp` 或 `.hpp`，必須同步更新 `files.cmake` 的 `SRC_FILES` 或 `INCLUDE_FILES`，否則 CMake 不會把新檔編進 target。

## 目錄導覽

- 修改 runtime 實作、遊戲流程、碰撞、renderer、物件生成時，先讀 `src/AGENTS.md`。
- 修改 public headers、class/type ownership、繼承介面時，先讀 `include/AGENTS.md`。
- 修改關卡 JSON、資產路徑、背景圖或 runtime resources 時，先讀 `Resources/AGENTS.md`。
- 查看舊架構時，先讀 `_legacy/AGENTS.md`。
- 必須碰 PTSD framework 時，先讀 `PTSD/AGENTS.md`。

## 重要注意事項

- `README.md` 還是 PTSD template 的基本說明，不代表目前遊戲架構。
- 工作樹已有許多既有修改、刪除、未追蹤檔；後續 agent 不應隨意 revert。
- `ExportedSprites/1-3/...` 有不少 deleted 狀態，這看起來是既有工作樹狀態，不是本文件造成。
- `LevelLoader::Load()` 找不到檔案會 throw，目前 `GameManager` 沒 catch。
- `GameManager::ChangeLevel()` 會 `ResetSceneObjects()`，再重新 init HUD/scene；切關時 session progress 只保存玩家 form，不保存 timer 或 score 以外狀態。
