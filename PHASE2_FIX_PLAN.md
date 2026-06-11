# Phase 2 修復計畫 — NES 馬力歐完全復刻（交辦版）

> 本文件是獨立交辦文件：執行的 agent 不需要任何先前對話脈絡，照本文件即可動工。
> 前置：Phase 1（核心 Bug + 物理手感）已完成，位於 git branch `claude/funny-sammet-be1583`。
> **動工前必讀**：repo 根目錄 `AGENTS.md`、`src/AGENTS.md`、`include/AGENTS.md`。

---

## 0. 專案速覽與鐵則

- C++17 / CMake / PTSD framework (v0.2) 的 2D Mario clone。
- **建置（必須 Debug，Release 直接 fatal）**：
  ```sh
  cmake -DCMAKE_BUILD_TYPE=Debug -B build
  cmake --build build
  ./build/OOPL_Mario
  ```
- 新增 `.cpp`/`.hpp` **必須**同步加進 `files.cmake` 的 `SRC_FILES`/`INCLUDE_FILES`。
- 世界座標 = NES 像素（`TILE_SIZE=16`）；`GAME_SCALE=3` 只在渲染時乘。**速度/重力不得多乘 3**。
- 物理常數統一放 `include/GameConstants.hpp`（Phase 1 已建立 `GRAVITY_RISE/GRAVITY_FALL/MAX_FALL_SPEED/PLAYER_*` 系列），不要再散落魔術數字。
- `Util::Input`：`IsKeyDown`（當幀按下）/ `IsKeyPressed`（按住）。**沒有 IsKeyUp**，需要放開偵測請自己記上一幀狀態（參考 `Player.cpp` 的 `m_JumpCut` 寫法）。
- `Util::Image` **沒有 runtime tint**；閃爍效果用 `SetVisible()` 切換（參考 `Player::UpdateStarInvincibility`）。
- **重要更正**：PTSD v0.2 **有音效 API**！`Util::BGM`（背景音樂，串流、同時僅一首，`Play(loop)/FadeIn/FadeOut/Pause/Resume/SetVolume 0-128`）與 `Util::SFX`（音效，記憶體載入，`Play(loop=0, duration=-1)`）。header 在 PTSD 的 `include/Util/BGM.hpp`、`include/Util/SFX.hpp`。先前文件說「無音效支援」是錯的。
- 每個批次完成後必須 `cmake --build build` 通過才算完成；交付時工作樹必須可編譯。
- 保持現有程式風格（中文註解、命名慣例）；不要 revert 不相干的既有修改。

### Phase 1 已完成（不要重做）
變動跳躍高度（雙重力）、終端落速、相機棘輪+左緣 clamp、隱形磚單向、殼殺敵 pass、頂磚殺上方敵人、Goomba 壓扁+走路動畫、龜翻面死亡、龜殼 5s 甦醒、食人花藏匿/避玩家、問號磚動畫+彈跳、踩擊/火球/碎磚/金幣計分、combo、浮動分數、星星閃爍、蘑菇不降級、FIRE 受傷直接變小、時間流速 ×2.5、死亡彈跳穩健化、水管輸入修正。

---

## Batch A — 關卡推進與過關結算（複雜，建議 Sonnet 4.6）

**現況**：碰旗杆 → `StartLevelClearSequence`（滑杆、走 6 格、消失）→ `LevelClearTransition` 顯示 overlay 3 秒 → **回標題畫面**。沒有下一關、沒有時間結算、旗子不會降。

**目標行為（原版 NES）**：
1. 玩家觸杆 → 旗子從杆頂降到杆底（與玩家下滑同時）。
2. 玩家落地後自動走向城堡，走到城堡門口消失（目前固定走 6 格，可接受；若背景圖有城堡門，以門的 X 為終點更佳）。
3. 進入結算：剩餘時間以每幀數十單位的速度遞減，同步把「每單位 50 分」加進分數（HUD 的 TIME 動態減、SCORE 動態加）。
4. 結算完 → 進入下一關的 `LevelIntro`（黑底 WORLD x-x + 剩餘命數）→ 開始下一關。

**關卡鏈**（hardcode 在 `GameManager` 即可，做成 `static const std::vector<std::pair<std::string,std::string>>`，元素為 `{levelName, worldLabel}`）：
```
"1-1"           → "1-1"
"1-2_ground_1"  → "1-2"
"1-3_ground_1"  → "1-3"
（1-3 過關 → 回標題；之後新增關卡只要加進這張表）
```

**實作指引**：
- 旗子下降：`src/FlagBlock.cpp` 目前只畫杆頂球。新增旗子 sprite child（資產找 `Resources/Asset/item/flag/`，有什麼用什麼；若只有 `ball.png` 就先只做玩家下滑，旗子留 TODO 註解並回報）。
- 時間結算：在 `GameManager` 新增 flow state（如 `LevelClearCountdown`），每幀 `m_TimeRemaining -= 結算速度`，每扣 1 單位 `m_Session.AddScore(50)`，扣到 0 後停 1 秒進下一關。
- 下一關切換：`EnterLevelIntro()` 目前讀 `m_SelectedInitialLevelName`，需把「目前關卡 index」存進 GameManager（或 `GameSession::PlayerProgress.levelName`，該欄位已存在但沒人用），過關時推進 index。
- **死亡重生也要照目前關卡**（現在死掉會回 `m_SelectedInitialLevelName`，過關推進後死掉不能跳回 1-1）。

**驗收**：1-1 過關 → 時間結算動畫 → WORLD 1-2 intro → 1-2 可玩；在 1-2 死掉重生在 1-2；1-3 過關回標題。HUD 的 WORLD 標籤跟著換。

---

## Batch B — Game Over / Time Up 表現（中等，建議 Sonnet 4.6 或 Haiku 4.5）

**現況**：命數歸零直接回標題，無 Game Over 畫面；Time Up 直接扣命無死亡動畫。

**目標**：
1. **Game Over 畫面**：命數歸零後顯示黑底「GAME OVER」約 3 秒再回標題（仿 `BuildTimeUpOverlay`/`UpdateTimeUp` 的 pattern：新 flow state + overlay + timer）。改 `HandleLifeLost()` 的 `IsGameOver()` 分支。
2. **Time Up 死亡動畫**：時間歸零時不要直接 `EnterTimeUp()` 清場，先讓玩家播死亡動畫（呼叫 `m_Player.Downgrade()` 直到死亡 or 直接設 Dying state），動畫播完才進 TIME UP overlay → 扣命。參考 `UpdatePlaying` 裡 `IsDying` 的處理流程。

**驗收**：故意死 3 次 → 看到 GAME OVER 畫面才回標題；等時間歸零 → 馬力歐播死亡跳 → TIME UP → 扣命重生。

---

## Batch C — 中繼點 Checkpoint（中等，建議 Sonnet 4.6）

**現況**：死亡永遠從 `playerSpawn` 重生。原版 1-1 過半後死亡會從中繼點重生。

**實作**：
1. `include/LevelData.hpp`：`LevelData` 加 `std::vector<glm::vec2> checkpoints;`（可選欄位）。
2. `src/LevelLoader.cpp`：解析 JSON 頂層 `"checkpoints": [{"x":...,"y":...}]`（缺省為空陣列）。
3. `GameManager`：玩家 X 越過 checkpoint X 時記錄「已達成的最後 checkpoint」（存活期間持續更新）；`HandleLifeLost` → `EnterLevelIntro` 重生時若有達成的 checkpoint 就用它當 spawn。**換關時清空**。
4. `Resources/data/1-1.json` 加一個中繼點（原版 1-1 中繼點約在關卡 56% 處；此關 `levelWidth=3584` → x≈2000，y 與 playerSpawn 同為 192）。
5. 同步更新 `JSON_LEVEL_TUTORIAL.md` 文件說明新欄位。

**驗收**：1-1 走過 x=2000 後死掉 → 從中繼點重生；沒走到就死 → 從頭重生；過關進下一關後 checkpoint 重置。

---

## Batch D — 音效與音樂（複雜，建議 Sonnet 4.6）

**框架**：用 PTSD 內建 `Util::BGM` / `Util::SFX`（確認存在於 PTSD v0.2，底層 SDL_mixer，建置樹已含）。
**資產**：`Resources/Asset/` 目前**沒有任何音檔**。請建立 `Resources/Asset/audio/bgm/` 與 `Resources/Asset/audio/sfx/` 目錄結構與載入程式碼，**音檔由專案擁有者自行放入**（檔名約定如下）。程式必須在音檔缺失時優雅降級（try-catch 或先檢查檔案存在，缺檔就靜音並 LOG_WARN，**不得 crash**）。

**實作**：
1. 新增 `include/AudioManager.hpp` + `src/AudioManager.cpp`（記得 `files.cmake`）：
   - 單例或由 `GameManager` 持有的成員。
   - `PlayBGM(name)` / `StopBGM()` / `PlaySFX(name)`；內部用 map 快取 `Util::SFX` 物件（SFX 是記憶體載入，重複建構浪費）。
   - 檔名約定（缺檔自動跳過）：
     - BGM：`overworld.mp3`、`underground.mp3`、`starman.mp3`、`level_clear.mp3`、`death.mp3`、`game_over.mp3`
     - SFX：`jump_small.wav`、`jump_super.wav`、`stomp.wav`、`coin.wav`、`powerup.wav`、`powerup_appears.wav`、`brick_break.wav`、`bump.wav`、`kick.wav`、`fireball.wav`、`pipe.wav`、`1up.wav`、`flagpole.wav`
2. 接點（全部在現有程式裡有明確位置）：
   - 跳躍：`Player::Jump()`
   - 踩敵/踢殼：`GameManager::CheckStompCollision`
   - 金幣：`SpawnItem` Coin 分支 + `CheckItemCollision` LevelCoin
   - 吃道具/道具冒出：`CheckItemCollision` + 問號磚 `OnHit`
   - 碎磚/頂磚：`CheckBlockCollision` 的 `hitBelowBlock` 區
   - 火球：`SpawnFireball`
   - 水管：`StartPipeEntry/Exit`
   - 1UP：`GameSession::AddLife` 呼叫處
   - 旗杆：`CheckFlagCollision`
   - BGM 切換：`LoadLevel`（依 `m_Level.theme` 選 overworld/underground）、吃星（starman BGM，結束切回）、死亡、過關、Game Over。
3. 星星 BGM 結束後要恢復關卡 BGM（在 `UpdateStarInvincibility` 計時歸零處通知）。

**驗收**：無音檔時遊戲正常跑（只有 LOG_WARN）；放入音檔後各事件正確發聲、地下關 BGM 不同、吃星換曲且 10 秒後切回。

---

## Batch E — Phase 1 遺留缺口（簡單～中等，建議 Haiku 4.5，第 5 項 Sonnet）

1. **拆分 combo 計數器**：`GameManager` 目前踩擊與殼殺共用 `m_ComboCount`。新增 `m_ShellComboCount` + `NextShellComboScore()`，`CheckShellEnemyCollision` 改用之；殼停止滑行或被消滅時歸零（在 `Koopa` 加 `WasSlidingStopped` 查詢，或每幀檢查「無滑行中殼」就歸零——後者簡單可接受）。
2. **踩擊 combo 落地歸零的時機檢查**：確認 `CheckStompCollision` 開頭的 `IsOnGround()` 歸零不會在「踩到敵人的同一幀」誤歸零（踩擊後玩家彈起，理論上安全；驗證連踩兩隻 Goomba 是 100+200）。
3. **地下主題 Goomba/Koopa 貼圖**：`Goomba`/`Koopa` 建構子寫死 `ground` 主題路徑，但資產有 `enemy/Goomba/underground/...`。把 `m_Level.theme` 傳進 `SpawnEnemy` → 建構子，依主題載圖（underground 缺 `shell_flip.png`，fallback `shell.png` 已有，保留）。
4. **紅龜貼圖**：`Resources/Asset/enemy/Koopa/` 下沒有紅龜資產。**資產工作**：從 `ExportedSprites/`（若有）或自製，放入 `ground/red/walk/walk-1.png`、`walk-2.png`（+reverse），`Koopa::LoadSprites` 的 Red 分支已寫好 `std::filesystem::exists` 偵測，放對路徑即自動生效。若無法取得資產，回報即可，程式不用改。
5. **食人花遮擋實機驗證**（需跑遊戲目視）：目前做法是食人花 zIndex 0.5 < 水管 sprite zIndex。實際啟動遊戲到 1-1 第一根可進水管（x=912）等食人花縮回，確認看不到；若仍可見，改用「裁切」方案：`PiranhaPlant::Draw` 在低於水管口的部分不畫（把 drawable 換成依露出比例裁切的 sprite，或最簡單：完全縮回時 `SetVisible(false)`，部分縮回時可接受微露）。
6. **HUD 金幣圖示**：HUD 金幣區目前是純文字 `x00`。在金幣數字左側加一個小金幣 `Util::Image`（`item/coin/coin-1.png`），位置參考 `HUD.cpp` 的 `kCoinX`。

---

## Batch F — 暫停與變身動畫（中等，建議 Sonnet 4.6）

1. **暫停**：按 Enter（NES Start）切換暫停。暫停時 `UpdatePlaying` 不更新任何物件與時間，只 `m_Renderer.Update()` + 顯示「PAUSED」overlay 文字。注意 Enter 也是標題畫面的開始鍵——只在 `Playing` state 處理；用 `IsKeyDown`（單幀）防止連發。
2. **變身動畫**：吃蘑菇/花、受傷縮小時，全場凍結 ~1 秒播變身閃爍：
   - 凍結：仿照 `IsDying` 的處理——`Player` 加 `State::Transforming`，期間 `UpdatePlaying` 不更新敵人/時間。
   - 表現：在 SMALL↔SUPER 兩個 size/sprite 間以 ~0.07s 間隔來回切換 6–8 次，結束時定格新形態。FIRE 變身（吃花）原版是調色盤閃爍，沒有 tint 的情況下用「目前形態 ↔ FIRE 形態 sprite 交替」近似。
   - 受傷縮小同理反向。受傷後的 2 秒無敵閃爍照舊（`StartDamageInvincibility` 在變身動畫結束後才開始計時）。
3. 變身期間玩家無敵（不會被重複觸發 Downgrade）。

**驗收**：吃蘑菇全場停 1 秒、看到大小交替動畫；受傷同理；暫停時敵人/時間全停，再按 Enter 恢復。

---

## 執行順序與相依

```
Batch E（獨立小修，可最先）
Batch B（獨立）
Batch C（獨立）
Batch A（建議在 B 之後做——共用 flow state pattern）
Batch F（獨立；變身動畫會碰 Player state machine，避免與 A 同時改）
Batch D（最後做——接點遍佈全部程式，等其他批次穩定後再接音效最省事）
```
同一時間只能有一個批次在改 `GameManager.cpp` / `Player.cpp`（幾乎每批都會碰），**請序列執行，不要平行改檔**。

## 全局驗收（全部完成後）

1. `cmake --build build` 零錯誤。
2. 一條龍試玩：標題 → 1-1 → 吃蘑菇（變身動畫）→ 過中繼點 → 故意死（中繼點重生）→ 過關（旗降+時間結算）→ 1-2 → 暫停/恢復 → 1-3 → 過關回標題。
3. 死滿 3 命看到 GAME OVER；等 TIME UP 看到死亡動畫。
4. 無音檔時無 crash；有音檔時各事件發聲正確。
5. 觀察 console log 無非預期 `LOG_WARN`。

## 注意：目前程式碼在哪

Phase 1 的所有修改在 worktree branch **`claude/funny-sammet-be1583`**（路徑 `Mario/.claude/worktrees/funny-sammet-be1583/`），**尚未合併回主 repo**（主 repo 在另一個 branch `codex/nes-title-screen`）。Phase 2 動工前請先決定：在同一 worktree 繼續，或先 merge 回主線再開工。測試時務必執行**該工作樹自己的** `build/OOPL_Mario`，跑錯執行檔會看不到任何修改。
