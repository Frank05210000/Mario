# OOPL_Mario Agent Notes

這份文件是給後續 agents 快速接手用的專案導覽。它整理目前程式碼的實際行為、檔案責任、資料流，以及修改時最容易踩到的點。

## 專案概況

- 類型：C++17 / CMake / PTSD framework 的 2D Mario-like 遊戲。
- 主程式：`src/main.cpp` 建立 `Core::Context` 與 `App`，依序跑 `Start -> Update -> End`。
- 核心遊戲邏輯：`GameManager` 集中管理流程狀態、關卡載入、物件更新、碰撞、HUD 與 renderer。
- runtime 關卡資料：目前從 `data/*.json` 載入，不是 `Resources/levels/*.json`。
- 資產根目錄：`Asset/`。程式用 `MakeAssetPath()` 將相對路徑接到 `RESOURCE_DIR` 的上一層 `Asset`。
- 內嵌 framework：`PTSD/`。除非是 framework 問題，通常不要改。
- 舊版程式：`_legacy/`。可作參考，但目前 build 使用 `src/` 與 `include/`。

## 建置與執行

Debug build 是必要條件，因為 `CMakeLists.txt` 只在 Debug 定義可用的 `RESOURCE_DIR`，Release 會直接 fatal。

```sh
cmake -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
./build/OOPL_Mario
```

目前已驗證 `cmake --build build` 可成功完成。

如果新增 `.cpp` 或 `.hpp`，必須同步更新 `files.cmake` 的 `SRC_FILES` 或 `INCLUDE_FILES`，否則 CMake 不會把新檔編進 target。

## 重要目錄

- `src/`：主要實作。
- `include/`：主要 header。
- `data/`：目前實際載入的 JSON 關卡。
- `Asset/`：實際使用的圖像、字體、音效等資產。
- `ExportedSprites/`：匯出的 sprite 素材/參考，不是目前程式直接使用的主要 asset root。
- `Resources/`：CMake 的 `RESOURCE_DIR` 指向這裡，但程式會回到上一層找 `data/` 與 `Asset/`。
- `PTSD/`：framework dependency。
- `_legacy/`：舊架構程式碼，不在目前 executable 的 `files.cmake` 內。

## Runtime Flow

1. `main.cpp`
   - 建立 `App`。
   - 每幀呼叫 `context->Update()`。
   - 根據 `App::State` 呼叫 `Start()`、`Update()` 或 `End()`。

2. `App`
   - `Start()` 呼叫 `GameManager::Start()`。
   - `Update()` 呼叫 `GameManager::Update()`。
   - ESC 或視窗關閉時進入 END。
   - `App` 本身不放遊戲規則。

3. `GameManager`
   - 狀態列舉：`Title`、`LevelIntro`、`Playing`、`TimeUp`、`LevelClearTransition`。
   - `Start()` 進 title screen。
   - title 按 Enter 或 Space 開始新遊戲。
   - `LevelIntro` 顯示約 2 秒後進 `Playing`。
   - `Playing` 每幀更新 player、enemy、block、item、fireball、debris，然後做 camera、spawn queue、碰撞、旗杆與 HUD。
   - 時間歸零進 `TimeUp`。
   - 碰到旗杆進過關動畫，完成後顯示 clear overlay，再回 title。

## 主要類別責任

### App / GameManager / Session

- `App`：PTSD app lifecycle adapter，只轉接給 `GameManager`。
- `GameManager`：所有物件生命週期與互動的中樞。
  - 持有 `Player`、`Camera`、`HUD`、`GameSession`。
  - 持有 `m_Blocks`、`m_Enemies`、`m_EnemySpawnQueue`、`m_Items`、`m_Fireballs`、`m_BrickDebris`。
  - 建立 renderer scene，並在 `DrawScene()` 讓各物件把 world coordinates 轉 screen coordinates。
- `GameSession`：玩家進度，包含 lives、score、coins、levelName、form。支援 1 或 2 player，但目前 `StartNewGame()` 固定 `ResetNewGame(1)`。

### Level Loading

- `LevelLoader::Load(path)` 讀 JSON，回傳純資料 `LevelData`。
- `LevelData` 包含 background image、theme、level size、player spawn、object list。
- `ObjectData` 是 JSON object 的中間資料，`GameManager::LoadLevel()` 依 `type` 建立真正物件。
- 目前主關卡路徑寫在 `src/GameManager.cpp`：
  - `kLevel1_1 = RESOURCE_DIR + "/../data/1-1.json"`
- 水管切換透過 `MakeLevelPath(levelName)` 轉到 `data/<levelName>.json`。

### Camera / Coordinates

- world origin 在左上角，x 向右，y 向下。
- `TILE_SIZE = 16`。
- `GAME_SCALE = 3`，世界 16x16 tile 畫成 48x48 screen pixels。
- PTSD screen coordinates 是以視窗中心為原點且 y 向上，所以所有場景物件都透過 `Camera::WorldToScreen()` 轉換。
- `Camera::Update(targetX, levelWidth)` 只水平追蹤玩家，並夾制在關卡左右邊界。

### Character / Player / Enemy

- `Character` 繼承 `Util::GameObject`，提供 position、velocity、size、alive、gravity 與 `ApplyGravity()`。
- `Player`
  - forms：`SMALL`、`SUPER`、`FIRE`。
  - controls：左右方向鍵或 A/D 移動，Space/W/Up 跳，Z 跑步，FIRE 形態按 Z 也會請求射火球。
  - debug shortcuts：1/2/3 切換 SMALL/SUPER/FIRE。
  - `Update()` 儲存 previous position，處理輸入、重力、死亡動畫、受傷無敵閃爍、過關自動走位與動畫切換。
  - `ConsumeShootRequest()` 由 `GameManager` 消耗，限制畫面最多 2 顆火球。
- `Enemy`
  - 共用走路、重力、轉向。
  - `Stomp()` 是純虛擬。
- `Goomba`
  - 16x16，被踩直接死亡。
- `Koopa`
  - 初始 16x32，建構時 y 會往上修正一格。
  - 被踩縮殼成 16x16。
  - 靜止龜殼可被踢，滑行速度約 380 px/s。
- `PiranhaPlant`
  - 不使用 block collision。
  - 在 hidden/rising/extended/lowering 四態之間循環。
  - `Stomp()` 目前不做事，視為 hazard。

### Blocks

- `Block` 繼承 `Util::GameObject`，提供 position、size、itemType、used state，以及 `OnHit(Player*)`。
- `GroundBlock`：不可見但 solid，通常覆蓋背景圖上的地板碰撞。
- `WallBlock`：可見且 solid，不會被敲開。
- `BrickBlock`
  - 可藏 item。
  - SMALL Mario 敲一般磚只 bounce。
  - SUPER/FIRE 敲空磚會 destroyed，`GameManager` 產生 `BrickDebris`。
  - 藏 item 的磚會變 used block 並 spawn item。
- `QuestionBlock`
  - 第一次敲出 item，之後變 used block。
  - itemType 空值時由 `GameManager` 預設為 `Coin`。
- `PipeBlock`
  - solid。
  - 支援 `opening`：`up`、`down`、`left`、`right`。
  - 可 enterable，並用 `targetLevel` 或 `exitToLevel` 切關。
  - `targetSpawn` 可覆蓋切關後玩家位置。
- `FlagBlock`
  - 非 solid。
  - JSON y 是旗杆頂端；碰撞高度由 `FLAG_POLE_TILES` 決定。
  - `GetContactScore()` 依碰撞高度給 5000/2000/800/400/100。
- `MovingPlatformBlock`
  - solid，沿 horizontal/vertical 在 start 到 distance 間往返。
  - 目前玩家站在平台上不會被平台主動 carry，只有碰撞推出。
- `TreePlatformBlock`
  - solid，使用多段 tree top sprite 組成 1-3 風格平台。

### Items / Fireballs / Effects

- `Item` 有 `Spawning`、`Active`、`Collected` 三態。
- `CoinItem`
  - 方塊敲出時立刻計入 coin。
  - 只播放上拋動畫，回落後自動 Collected。
  - 不靠玩家接觸收集。
- `MushroomItem`
  - 從方塊冒出後變 Active，往右走並受重力。
  - 玩家接觸後 `Player::SetForm(SUPER)`。
- `FireFlowerItem`
  - 從方塊冒出後靜止。
  - 玩家接觸後 `Player::SetForm(FIRE)`。
- `Fireball`
  - FIRE Mario 射出。
  - 受重力，碰地反彈，撞牆/天花板爆炸。
  - 擊中敵人會讓敵人 dead/invisible，自己爆炸。
- `BrickDebris`
  - SUPER/FIRE Mario 打碎磚塊時產生四片，約 0.9 秒後消失。

### HUD

- `HUD::Init(renderer)` 建立並加入固定螢幕座標文字。
- `HUD::Update(score, coins, timeLeft)` 更新分數、金幣與倒數。
- 字體：`Asset/font/Super Mario Bros. NES.ttf`。
- 目前世界文字固定為 `1-1`。

## JSON Level Format

詳細格式看 `JSON_LEVEL_TUTORIAL.md`。核心欄位：

```json
{
  "backgroundImage": "level_image/1-1/ground.png",
  "theme": "ground",
  "levelWidth": 3584,
  "levelHeight": 240,
  "playerSpawn": { "x": 32.0, "y": 176.0 },
  "objects": []
}
```

支援 object types：

- `Ground`
- `Wall`
- `Brick`
- `QuestionBlock`
- `EnemySpawn`
- `Pipe`
- `Flag`
- `MovingPlatform`
- `TreePlatform`

支援 enemy types：

- `Goomba`
- `Koopa`
- `PiranhaPlant`

常見 item types：

- `Coin`
- `PowerUp`
- `Mushroom`
- `None`

`PowerUp` 在 `GameManager::SpawnItem()` 內依玩家形態決定：SMALL 產生 mushroom，SUPER/FIRE 產生 fire flower。

## Collision Model

- 大多數碰撞用 `CollisionUtils::CheckAABB()`。
- 玩家與 block 碰撞由 `GameManager::CheckBlockCollision()` 處理。
  - 使用 current position、previous position、velocity 與 penetration 判斷從上落地、從下敲擊或側碰。
  - 從下撞到 block 會呼叫 `block->OnHit(&m_Player)`。
  - `BlockHitResult` 回傳 destroyed 或 spawnItem，`GameManager` 負責真正移除/生成。
- enemy/item/fireball 與 block 有各自的 collision pass。
- 玩家踩 enemy 由 `CheckStompCollision()` 處理。
  - 必須是向下速度，且 previous bottom 在 enemy top 附近。
  - 靜止 Koopa shell 被踩或側碰會被 kick。
  - 其他側碰會讓玩家 `Downgrade()`，若沒死則啟動短暫 damage invincibility。
- 掉出 `levelHeight + 50` 會判定死亡或清除。

## Renderer Pattern

- 場景物件通常繼承 `Util::GameObject`。
- 新物件建立後要加入 `m_Renderer.AddChild(...)`，否則不會畫。
- `BuildScene()` 加入 background、player、enemies、blocks、fireballs、debris。
- items、fireballs、debris 若在 runtime 生成，生成函式會立即 `AddChild()`。
- `Player` 是 stack member，加入 renderer 時用 `std::shared_ptr<Player>(&m_Player, [](Player*){})` 避免 renderer 刪除它。
- 複合物件如 `PipeBlock`、`TreePlatformBlock` 內部用 child `GameObject` 組段，並 override `Draw()` 更新每段位置。

## 新增功能的建議流程

### 新增 Block

1. 新增 `include/NewBlock.hpp` 與 `src/NewBlock.cpp`，繼承 `Block`。
2. 在 `Block::Type` 加新 enum 值。
3. 實作 `GetType()`、`IsSolid()`，必要時 override `Update()`、`Draw()`、`OnHit()`。
4. 在 `GameManager::LoadLevel()` 加 JSON `type` 到 class 的建立邏輯。
5. 更新 `files.cmake`。
6. 更新 `JSON_LEVEL_TUTORIAL.md` 與 `AGENTS.md`。

### 新增 Enemy

1. 繼承 `Enemy`。
2. 實作建構子、`Stomp()`，必要時 override `Update()` 或 `UsesBlockCollision()`。
3. 在 `GameManager::SpawnEnemy()` 加 `enemyType` mapping。
4. 更新 JSON 文件與 `files.cmake`。

### 新增 Item

1. 繼承 `Item`。
2. 實作 `Update()`、`OnCollect()`、`GetType()`。
3. 在 `GameManager::SpawnItem()` 加 itemType mapping。
4. 如果由 block 產生，確認 `BlockHitResult::spawnItem` 與 JSON `itemType` 字串一致。

### 新增關卡

1. 在 `data/` 新增 `<name>.json`。
2. 背景圖放在 `Asset/level_image/...`，JSON 的 `backgroundImage` 用相對 `Asset/` 的路徑，例如 `level_image/1-1/ground.png`。
3. 若要從水管切換，`targetLevel` 可寫不含 `.json` 的名稱。
4. 確認 `levelWidth`、`levelHeight` 與背景圖/碰撞物件一致。

## 已知注意事項

- Release build 目前不可用，請用 Debug。
- `Resources/levels/` 和 `data/` 同時存在，但目前實際載入的是 `data/`。
- `README.md` 還是 PTSD template 的基本說明，不代表目前遊戲架構。
- 工作樹已有許多既有修改/刪除/未追蹤檔，後續 agent 不應隨意 revert。
- `ExportedSprites/1-3/...` 有不少 deleted 狀態，這看起來是既有工作樹狀態，不是本文件造成。
- HUD 世界名稱固定 `1-1`，如果加入多關卡顯示，需要讓 HUD 接收目前 level name。
- `Player::ClampToCameraBounds()` 有實作但目前 `GameManager::UpdatePlaying()` 未呼叫。
- `GameSession` 支援 2 players，但目前 new game 固定 1 player。
- `PiranhaPlant::Stomp()` 目前空實作，玩家若從上方踩到它仍可能進入 stomp collision 分支而沒有傷害；要精準 hazard 行為需調整 collision logic。
- `MovingPlatformBlock` 不會 carry player，站上移動平台時可能需要額外位移補償。
- `LevelLoader::Load()` 找不到檔案會 throw，目前 `GameManager` 沒 catch。
- `GameManager::ChangeLevel()` 會 `ResetSceneObjects()`，再重新 init HUD/scene；注意切關時 session progress 只保存玩家 form，不保存 timer 或 score以外狀態。

## 目前驗證

- `cmake --build build`：通過。

