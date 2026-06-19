# src Agent Notes

這份文件描述目前 runtime 實作、資料流、碰撞與 renderer pattern。新增或修改遊戲行為時優先讀這份。

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

## GameManager 與資料載入

- `GameManager` 是所有物件生命週期與互動的中樞。
  - 持有 `Player`、`Camera`、`HUD`、`GameSession`。
  - 持有 `m_Blocks`、`m_Enemies`、`m_EnemySpawnQueue`、`m_Items`、`m_Fireballs`、`m_BrickDebris`。
  - 建立 renderer scene，並在 `DrawScene()` 讓各物件把 world coordinates 轉 screen coordinates。
- `LevelLoader::Load(path)` 讀 JSON，回傳純資料 `LevelData`。
- `ObjectData` 是 JSON object 的中間資料，`GameManager::LoadLevel()` 依 `type` 建立真正物件。
- 目前主關卡路徑寫在 `src/GameManager.cpp`：
  - `kLevel1_1 = RESOURCE_DIR + "/data/1-1.json"`
- 水管切換透過 `MakeLevelPath(levelName)` 轉到 `Resources/data/<levelName>.json`。

## Camera / Coordinates

- world origin 在左上角，x 向右，y 向下。
- `TILE_SIZE = 16`。
- `GAME_SCALE = 3`，世界 16x16 tile 畫成 48x48 screen pixels。
- PTSD screen coordinates 是以視窗中心為原點且 y 向上，所以所有場景物件都透過 `Camera::WorldToScreen()` 轉換。
- `Camera::Update(targetX, levelWidth)` 只水平追蹤玩家，並夾制在關卡左右邊界。

## Collision Model

- 大多數碰撞用 `CollisionUtils::CheckAABB()`。
- 玩家與 block 碰撞由 `GameManager::CheckBlockCollision()` 處理。
  - 使用 current position、previous position、velocity 與 penetration 判斷從上落地、從下敲擊或側碰。
  - 從下撞到 block 會呼叫 `block->OnHit(&m_Player)`。
  - `BlockHitResult` 回傳 destroyed 或 spawnItem，`GameManager` 負責真正移除/生成。
- enemy/item/fireball 與 block 有各自的 collision pass。
- 玩家踩 enemy 由 `CheckStompCollision()` 處理。
  - 必須是向下速度，且 previous bottom 在 enemy top 附近。
  - `Enemy::Stomp()` 回傳 `StompOutcome`；飛行龜掉翅膀、Koopa 縮殼、滑殼停止分開處理。
  - 靜止 Koopa shell 從側面碰到會被 kick；從上方踩也會依踩踏位置 kick，踩在中心附近時依玩家面向決定方向。
  - 滑行殼的連殺計數由每一顆 Koopa 自己持有，停止或死亡時才重置。
  - 其他側碰會讓玩家 `Downgrade()`，若沒死則啟動短暫 damage invincibility。
- 掉出 `levelHeight + 50` 會判定死亡或清除。
- 紅 Koopa 只有在 `Walking` 且 grounded 時檢查前方支撐；無支撐會轉向。綠龜與滑殼會掉下平台。
- 翻面死亡的 Koopa 保持 alive 以繼續重力與繪製，但 `CanCollide()` 為 false，掉出 kill Z 後才清除。

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
6. 更新 `JSON_LEVEL_TUTORIAL.md`、`src/AGENTS.md`、`include/AGENTS.md`，必要時更新 `Resources/AGENTS.md`。

### 新增 Enemy

1. 繼承 `Enemy`。
2. 實作建構子、`Stomp()`，必要時 override `Update()` 或 `UsesBlockCollision()`。
3. 在 `GameManager::SpawnEnemy()` 加 `enemyType` mapping。
4. 更新 JSON 文件、`files.cmake` 與相關 `AGENTS.md`。

### 新增 Item

1. 繼承 `Item`。
2. 實作 `Update()`、`OnCollect()`、`GetType()`。
3. 在 `GameManager::SpawnItem()` 加 itemType mapping。
4. 如果由 block 產生，確認 `BlockHitResult::spawnItem` 與 JSON `itemType` 字串一致。

## 已知 gameplay 注意事項

- HUD 世界名稱固定 `1-1`，如果加入多關卡顯示，需要讓 HUD 接收目前 level name。
- `Player::ClampToCameraBounds()` 有實作但目前 `GameManager::UpdatePlaying()` 未呼叫。
- `GameSession` 支援 1P/2P 輪流進度；`GameManager` 仍只建立一個 `Player`，輪到 Luigi 時切換同一物件的外觀與 session 進度，不同場建立兩個 player。
- `PiranhaPlant::Stomp()` 目前空實作，玩家若從上方踩到它仍可能進入 stomp collision 分支而沒有傷害；要精準 hazard 行為需調整 collision logic。
- `MovingPlatformBlock` 已有 carry player 行為；修改平台碰撞時注意 `MovingPlatformBlock` 的 frame delta 補償。
