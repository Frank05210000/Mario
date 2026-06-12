# include Agent Notes

這份文件描述 public headers、主要類別責任與繼承介面。修改 `.hpp` 或新增 public type 時優先讀這份。

## Header 與 build 規則

- 新增 `.hpp` 必須同步更新頂層 `files.cmake` 的 `INCLUDE_FILES`。
- 新增對應 `.cpp` 時也要更新 `SRC_FILES`。
- public type 的 ownership 與資料流應與現有 headers 一致，不要只在 `.cpp` 暗中新增跨模組契約。

## App / GameManager / Session

- `App`：PTSD app lifecycle adapter，只轉接給 `GameManager`。
- `GameManager`：所有物件生命週期與互動的中樞。
  - 持有 `Player`、`Camera`、`HUD`、`GameSession`。
  - 持有 `m_Blocks`、`m_Enemies`、`m_EnemySpawnQueue`、`m_Items`、`m_Fireballs`、`m_BrickDebris`。
- `GameSession`：玩家進度，包含 lives、score、coins、levelName、form。支援 1 或 2 player，但目前 `StartNewGame()` 固定 `ResetNewGame(1)`。

## Level Data

- `LevelData` 包含 background image、theme、level size、player spawn、object list。
- `ObjectData` 是 JSON object 的中間資料，`GameManager::LoadLevel()` 依 `type` 建立真正物件。
- 修改 level schema 時要同步更新 `Resources/AGENTS.md` 與 `JSON_LEVEL_TUTORIAL.md`。

## Character / Player / Enemy

- `Character` 繼承 `Util::GameObject`，提供 position、velocity、size、alive、gravity 與 `ApplyGravity()`。
- `Player`
  - forms：`SMALL`、`SUPER`、`FIRE`。
  - controls：左右方向鍵或 A/D 移動，Space/W/Up 跳，Z 跑步，FIRE 形態按 Z 也會請求射火球。
  - debug shortcuts：1/2/3 切換 SMALL/SUPER/FIRE。
  - `Update()` 儲存 previous position，處理輸入、重力、死亡動畫、無敵配色顯示、過關自動走位與動畫切換。
  - `ConsumeShootRequest()` 由 `GameManager` 消耗，限制畫面最多 2 顆火球。
- `Enemy`
  - 共用走路、重力、轉向。
  - `Stomp()` 是純虛擬並回傳 `StompOutcome`，呼叫端依結果處理得分與互動。
  - 提供 `CanCollide()` 分離「仍需更新/繪製」與「仍可碰撞」，並記錄 grounded 狀態。
- `Goomba`：16x16，被踩直接死亡。
- `Koopa`
  - 狀態為 `Walking`、`ShellIdle`、`ShellReviving`、`ShellSliding`、`Defeated`。
  - 初始 16x32；縮殼後 16x16 並保持腳底位置。滑殼被踩會停止，靜止殼側碰會被踢。
  - 靜止殼約 5 秒後復活，最後 1.5 秒顯示復活警告圖。
  - `Defeated` 狀態仍更新翻面拋物線，但 `CanCollide()` 為 false。
- `KoopaParatroopa`：第一次踩踏只掉翅膀並變回同色 Koopa，第二次才縮殼。
- `PiranhaPlant`：不使用 block collision；在 hidden/rising/extended/lowering 四態之間循環；`Stomp()` 目前不做事，視為 hazard。

## Blocks

- `Block` 繼承 `Util::GameObject`，提供 position、size、itemType、used state，以及 `OnHit(Player*)`。
- `GroundBlock`：不可見但 solid，通常覆蓋背景圖上的地板碰撞。
- `WallBlock`：可見且 solid，不會被敲開。
- `BrickBlock`：可藏 item；SMALL Mario 敲一般磚只 bounce；SUPER/FIRE 敲空磚會 destroyed；藏 item 的磚會變 used block 並 spawn item。
- `QuestionBlock`：第一次敲出 item，之後變 used block；itemType 空值時由 `GameManager` 預設為 `Coin`。
- `PipeBlock`：solid；支援 opening、enterable、target level、target spawn。
- `FlagBlock`：非 solid；JSON y 是旗杆底部；內部用 `FLAG_POLE_TILES` 往上推算碰撞高度；`GetContactScore()` 依碰撞高度給分。
- `MovingPlatformBlock`：solid，沿 horizontal/vertical 移動；支援 `oscillate` 與 `verticalWrap`。
- `TreePlatformBlock`：solid，使用多段 tree top sprite 組成 1-3 風格平台。
- `HiddenBlock`：初始 invisible 但 solid；玩家從下方撞擊後顯示 used block 並依 `itemType` spawn item。
- `MultiCoinBlock`：16x16 solid，可用 `coinCount` 控制可敲出 coin 數量，預設 10；耗盡後變 used block。

## Items / Fireballs / Effects

- `Item` 有 `Spawning`、`Active`、`Collected` 三態。
- `CoinItem`：方塊敲出時立刻計入 coin；只播放上拋動畫，回落後自動 Collected。
- `MushroomItem`：從方塊冒出後變 Active，往右走並受重力；玩家接觸後 `Player::SetForm(SUPER)`。
- `FireFlowerItem`：從方塊冒出後靜止；玩家接觸後 `Player::SetForm(FIRE)`。
- `LevelCoinItem`：JSON 直接放置的 open coin；初始 Active，不受重力；玩家接觸後加 coin/score。
- `OneUpMushroomItem`：行為接近 mushroom，收集後 `GameSession::AddLife()`。
- `StarmanItem`：從方塊冒出後移動並彈跳；收集後 `Player::ActivateStarInvincibility()`；star 狀態碰敵人會直接擊殺敵人。
- `Fireball`：FIRE Mario 射出；受重力，碰地反彈，撞牆/天花板爆炸；擊中敵人會讓敵人 dead/invisible，自己爆炸。
- `BrickDebris`：SUPER/FIRE Mario 打碎磚塊時產生四片，約 0.9 秒後消失。

## HUD

- `HUD::Init(renderer)` 建立並加入固定螢幕座標文字。
- `HUD::Update(score, coins, timeLeft)` 更新分數、金幣與倒數。
- 字體：`Resources/Asset/font/Super Mario Bros. NES.ttf`。
- 目前世界文字固定為 `1-1`。
