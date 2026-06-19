# OOPL Mario

以 C++17、CMake 與 PTSD framework 製作的 2D Mario-like 遊戲。遊戲包含 `1-1`、`1-2`、`1-3` 三個主要關卡，以及由水管連接的地下區域。

## 建置與執行

專案目前必須使用 Debug build，因為 `RESOURCE_DIR` 只在 Debug 組態中定義。

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
./build/OOPL_Mario
```

執行測試：

```sh
./build/KoopaBehaviorTests
```

## 操作方式

- `A`／`D` 或左右方向鍵：移動
- `Space`、`W` 或上方向鍵：跳躍
- `Z`：奔跑；Fire Mario 可發射火球
- `Enter`／`Space`：在標題畫面開始遊戲
- `Esc`：開啟或關閉暫停選單

Debug mode 可在暫停選單中切換。啟用後，數字鍵 `1`～`3` 可切換關卡，`4`～`6` 可切換角色形態，`7` 可切換無限星星狀態，`8` 可增加生命。

## 專案結構

- `src/`、`include/`：遊戲流程、物件、碰撞、渲染與音效實作
- `Resources/data/`：runtime 關卡 JSON
- `Resources/Asset/`：runtime 圖片、字型與音效
- `tests/`：Koopa 行為與必要資產測試
- `PTSD/`：課程 framework dependency
- `JSON_LEVEL_TUTORIAL.md`：關卡 JSON 格式說明
- `2026OOPL_Final.md`：期末專案報告

新增 `.cpp` 或 `.hpp` 時，必須同步更新 `files.cmake`。
