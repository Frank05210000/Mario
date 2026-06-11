# Bug 報告與修復計畫

目標：將本專案修到與原版 NES《Super Mario Bros.》(SMB1) 行為一致。
本文件記錄目前發現的所有問題（含出處 `檔案:行號`），並提供分階段修復計畫。

> 產生日期：2026-06-11
> 研究範圍：`src/`、`include/` 全部 runtime 程式碼 + `Resources/data/*.json`

---

## 第一部分：問題清單

### A. 程式 Bug（不論是否復刻都是錯的）

#### A-0. 貼牆跳（牆面可以當地板跳）★最高優先
- **現況**：貼著牆下落時按住方向鍵，每下落 16px 就可以跳一次，等於可以沿牆爬升。
- **原因**：`src/GameManager.cpp:925` 的 `landedFromAbove` 只驗證垂直歷史
  （上一幀腳底 ≤ 方塊頂 + 2px），**沒有**檢查 `penY < penX` 或 `overlapX` 門檻。
  貼牆時每幀會往牆內擠進幾 px 產生 AABB 重疊；腳底每越過一格牆磚的頂邊，
  該牆磚就被誤判為「從上方落地」→ `onGround = true` 一幀 → 可跳。
- **根因**：單一迴圈混合 X/Y 軸解析（與 A-1 穿牆同源），靠容差猜碰撞方向。
- **原版行為**：牆面只會水平推回，永遠不給落地判定。

#### A-1. 無終端落速 → 高速下落穿牆
- **現況**：`src/Character.cpp` `ApplyGravity()` 對 `m_Velocity.y` 無上限。
  長距離下落後每幀位移超過 16px（一格），`landedFromAbove` 的 2px 容差失效，
  落入側碰分支被水平推出 → 穿過磚塊。`standingOverlap`（`src/GameManager.cpp:933`）
  只補救 `Ground` 型方塊，磚塊、水管照穿。
- **原版行為**：落速上限 4px/frame（≈ 240 px/s）。

#### A-2. 隱形磚塊永遠是實心的
- **現況**：`include/HiddenBlock.hpp:13` `IsSolid()` 恆回傳 `true`。
  走路會撞到看不見的牆、可以站在還沒敲出來的隱形磚上。
- **原版行為**：隱藏磚只有「玩家上升、從下方頂到」那一刻才有實體。
- **附註**：`grep -r "Hidden" Resources/data/` 為空——**目前沒有任何關卡 JSON 使用
  HiddenBlock**，這個 class 是寫好但未使用的程式碼，所以這個 bug 尚未在遊戲中現身。
  反過來說，原版 1-1 中段（第一個坑附近）有一個隱藏 1-UP 磚，是目前關卡資料的缺漏。
  用法見附錄一。

#### A-3. 火焰馬力歐吃蘑菇會被降級
- **現況**：`src/MushroomItem.cpp:46` `OnCollect()` 無條件 `SetForm(SUPER)`。
  情境：小馬力歐敲出蘑菇（生成時依當下形態決定，`src/GameManager.cpp:1076`），
  蘑菇在地上跑的期間玩家先吃花變 FIRE，再碰到這顆蘑菇 → 降級回 SUPER。
- **原版行為**：蘑菇永遠不會降級，已是 Super/Fire 時只加分。

#### A-4. 跳躍鍵與「向上進水管」衝突
- **現況**：`src/GameManager.cpp:643` `CheckPipeTransition` 的 `pressingUp` 用 UP/W，
  與跳躍鍵（`src/Player.cpp:371`）相同。在向下開口的水管旁跳躍會被吸進水管。

#### A-5. 進水管判定用單幀觸發
- **現況**：`src/GameManager.cpp:640-651` 全部用 `IsKeyDown`（這一幀剛按下）。
  按住「下」走到水管口上不會觸發，必須站定後重新按。
- **原版行為**：站上水管口時按住下即進入。

#### A-6. 紅龜與綠龜共用同一套貼圖
- **現況**：`src/Koopa.cpp:17` `LoadSprites()` 忽略參數（`(void)theme`），
  路徑寫死 `enemy/Koopa/ground/...`，`Variant::Red` 與 Green 外觀完全相同。
  （行為差異「紅龜不走出懸崖」已實作於 `src/GameManager.cpp:1013-1033`。）

#### A-7. 垂直巡邏的飛行龜會持續往左飄
- **現況**：`src/KoopaParatroopa.cpp:22` `VerticalPatrol` 模式仍執行
  `m_Position.x += m_Velocity.x * deltaTime`，而 `Enemy` 建構子給了 -40 初速。
- **原版行為**：紅飛龜定點上下飛，無水平位移。

#### A-8. 食人花縮回水管時畫在水管前面
- **現況**：`src/PiranhaPlant.cpp:17` zIndex = 6，但水管圖案畫在背景圖（zIndex = -1），
  中間沒有遮擋層 → 食人花「躲藏」時整株顯示在水管圖案上滑動，藏不起來。
  且躲藏時仍 `IsAlive()`，火球可以打死縮在水管裡的食人花。
- **原版行為**：縮回後完全不可見、不可互動。

#### A-9. 食人花不會避開玩家
- **現況**：`src/PiranhaPlant.cpp:27` 固定週期升降，會從玩家腳下冒出造成必死。
- **原版行為**：馬力歐站在水管上或緊鄰水管口時，食人花不會冒出來。

#### A-10. 死亡彈跳有機率不觸發
- **現況**：`src/Player.cpp:102` 死亡動畫依賴 `m_DeathTimer` 落在 `[0.5, 0.55)`
  的 0.05 秒視窗內才給向上初速；幀時間若跨過視窗（低幀率），馬力歐死亡會
  原地直接下墜，沒有經典死亡彈跳。應改為狀態切換時直接賦值。

#### A-11. 鏡頭可倒退 + 玩家可離開畫面左緣
- **現況**：`Player::ClampToCameraBounds()`（`src/Player.cpp:421`）寫了但
  `GameManager::UpdatePlaying()` 從未呼叫；`src/Camera.cpp:6` 鏡頭雙向置中跟隨，
  可以倒退捲動整張地圖。
- **原版行為**：鏡頭只進不退，玩家不能往回走出畫面左緣。

#### A-12. 變身作弊鍵留在正式輸入
- **現況**：`src/Player.cpp:294-300` 按 1/2/3 直接切換 SMALL/SUPER/FIRE。
  開發後門，復刻版必須移除（或移到 debug build 專用旗標後面）。

### B. 核心手感與原版不符

#### B-13. 跳躍物理整套不對（離復刻最遠的一塊）
- 無 **variable jump height**：原版按住 A 上升期重力小、放開（或開始下降）後
  重力約 3 倍；本作 `src/Player.cpp:384` 固定初速 320 + 單一重力 700，飄浮且不可控。
- 跳躍高度**不隨跑速增加**：原版依水平速度分 3 組（初速/重力都不同）。
- 無踩敵後按住跳可彈更高（現為固定 -180，`src/GameManager.cpp:782`）。

#### B-14. 速度數值換算錯誤（多乘了 GAME_SCALE）
- `include/Player.hpp:141-146` 的註解「×16×3」概念錯誤：世界座標本來就是
  NES 像素（TILE_SIZE = 16），渲染時才乘 `GAME_SCALE = 3`，**世界速度不該再乘 3**。
- 對照表（原版 → 本作現值）：
  | 項目 | 原版 (px/s) | 本作現值 | 出處 |
  |---|---|---|---|
  | 走路極速 | ≈ 90 | 87.5（接近）| `include/Player.hpp:141` |
  | 跑步極速 | ≈ 150 | 175（偏快）| `include/Player.hpp:142` |
  | 龜殼滑行 | ≈ 180 | **380（兩倍快）** | `src/Koopa.cpp:78` |
  | Goomba | ≈ 36 | 40（可接受）| `include/Enemy.hpp:52` |
  | 蘑菇 | ≈ 36 | 50（偏快）| `src/MushroomItem.cpp:28` |
  | 火球 | ≈ 240 | 320（偏快）| `src/Fireball.cpp:36` |

#### B-15. 重力常數四散且互不一致
- `Character` 700（`include/Character.hpp:68`）、`GameConstants.hpp` 的
  `GRAVITY = 900`（**沒有任何地方使用**）、`Item` 900（`include/Item.hpp:50`）、
  `Fireball` 1400、`CoinItem` 1500。應統一收進 `GameConstants.hpp` 管理。

#### B-16. 計分系統大量缺漏
- 踩死敵人 **0 分**（`src/GameManager.cpp:777` 只呼叫 `Stomp()`）。
  原版：Goomba 100、Koopa 100、踢殼 400、連踩 combo 翻倍（100→200→400→500→800→1000→2000→4000→5000→8000→1UP）。
- 火球殺敵 0 分（`src/GameManager.cpp:1297`）。
- 敲磚出金幣只 `AddCoin()` 不加 200 分（`src/GameManager.cpp:1075`）；
  但場景金幣有 200（`src/GameManager.cpp:1129`）——原版兩者都是 200。
- 敲碎磚塊無 50 分。
- 無浮動分數文字（"100"、"1UP" 跳字）。

#### B-17. 踩敵表現不完整
- Goomba 被踩**立即消失**（`src/Goomba.cpp:17`），無壓扁貼圖與約 0.4 秒停留。
- 被踢飛/火球殺/星星殺的敵人直接 `SetVisible(false)`，無「翻面落下」死亡動畫。
- 龜縮殼後**永遠不會甦醒**（原版約 5 秒後伸腳爬出，再過幾秒復原行走）。

#### B-18. 滑行龜殼不殺敵
- **現況**：無任何 enemy-vs-enemy 碰撞 pass，被踢出的殼穿過 Goomba 無效果，
  敵人彼此相撞也不回頭。
- **原版行為**：滑行殼是清版武器（每殺一隻分數翻倍）；行走敵人互撞會反向。

#### B-19. 頂磚不影響上方的敵人與道具
- **現況**：磚塊 bounce（`src/BrickBlock.cpp:15`）純視覺。
- **原版行為**：從下面敲磚可頂死站在磚上的敵人（得 100 分）、把道具彈起。

### C. 規則與流程

#### C-20. 受傷降級規則錯誤
- `src/Player.cpp:490` `FIRE → SUPER → SMALL` 是後代作品規則；
  **原版 SMB1 火焰馬力歐受傷直接變小**。另外變大/變小/變身都沒有
  變身動畫與全場暫停（原版會凍結全場播變身動畫）。

#### C-21. 無敵星沒有視覺回饋
- `src/Player.cpp:190` 只倒數計時，玩家不閃爍變色，無法判斷剩餘無敵時間。
  原版以調色盤循環閃爍，結束前變慢提示。

#### C-22. 時間流速錯誤
- `src/GameManager.cpp:218` 以真實秒 1:1 扣；原版 1 時間單位 ≈ 0.4 秒
  （400 時間 ≈ 160 真實秒），本作慢了 2.5 倍。

#### C-23. 過關流程與原版不符
- 過關後回標題畫面而非進下一關（`src/GameManager.cpp:241`）。
- 無「剩餘時間 × 50」結算倒數、旗子不會降下、無城堡升旗/煙火、
  馬力歐走固定 6 格就消失（`src/Player.cpp:129`）而不是走進城堡門。
- 旗杆分數區間 100/400/800/2000/5000 已符合原版（`src/FlagBlock.cpp:60`）✓。

#### C-24. 無中繼點（checkpoint）
- 死掉永遠從關卡開頭重生；原版過半後從中繼點重生。

#### C-25. Time Up / Game Over 表現缺失
- Time Up 直接扣命（`src/GameManager.cpp:229`），原版會讓馬力歐播死亡動畫。
- 無 GAME OVER 畫面（`src/GameManager.cpp:481` 直接回標題）。

#### C-26. 無音效音樂、無暫停
- 全 codebase 無任何 audio 呼叫；原版 Start 鍵可暫停（含音效）。

#### C-27. 敵人生成為一次性
- `src/GameManager.cpp:682` 進入鏡頭右緣 +32px 觸發一次後從 queue 移除；
  原版敵人離開畫面後再回來會重生。（在 A-11 修好「鏡頭只進不退」後，
  此項影響變小，可降級為低優先。）

### D. HUD 與原版差異

現況（`src/HUD.cpp`）：四個獨立 `Util::Text` 物件、座標手調
（`kScoreX = -kHalfW + 110` 等）、金幣顯示純文字 `x00`、靠 `\n` 換行排版。

原版正確版面（NES 畫面 256×224，HUD 佔頂部約兩行 tile，全程常駐含標題畫面）：

```
MARIO        (coin)x00    WORLD    TIME
000000                     1-1      400
```

差異項目：
1. **無金幣圖示**：原版「TIME 左側」是一顆會閃爍動畫的金幣 sprite + `x00`；本作是純文字。
2. **欄位不對齊 tile 網格**：原版每個字元落在 8px 網格上，四欄位置固定；
   本作座標是手調像素值，與原版排版位置不同。
3. **雙行排版方式**：原版上行標題、下行數值是兩個獨立定位的行；
   本作用 `\n` 包在同一個 Text 物件裡，行距由字型決定，與原版不一致。
4. **玩家名稱**：恆為 `MARIO`（`src/HUD.cpp:50`），`GameSession` 支援 Luigi 但 HUD 沒接。
5. **WORLD 顯示**：固定使用開局選的 label，水管進入子關卡（如 1-2 地下）後不更新。
6. **TIME 行為**：原版在非遊玩畫面（標題）不顯示數值；本作標題畫面無 HUD（可接受），
   但 Time Up 畫面排版與原版不同。

---

## 第二部分：修復計畫

依「先地基後表皮」排序；每個 Phase 結尾有驗收標準，全部通過才進下一階段。

### Phase 1：物理底層（最優先）

涉及：`src/Player.cpp`、`include/Player.hpp`、`src/Character.cpp`、
`include/Character.hpp`、`include/GameConstants.hpp`

1. **終端落速**：`ApplyGravity()` 夾制 `m_Velocity.y ≤ 240 px/s`（≈ 4px/frame）。
2. **Variable jump height**：起跳給初速；按住跳躍鍵期間用低重力，
   放開或 `vel.y > 0` 後切高重力（約 3 倍）。
3. **跑速分級跳躍**：依 |vel.x| 分 2~3 組初速，全速跑跳得更高。
4. **重力統一**：所有重力常數收進 `GameConstants.hpp`
   （玩家上升/下降、敵人、道具、火球各自具名常數），刪除散落的 magic number。
5. **速度表修正**：走 90 / 跑 150 / 殼 180 / 蘑菇 36 / 火球 240；
   修正 `include/Player.hpp:141` 起的錯誤換算註解（世界速度不乘 GAME_SCALE）。

**驗收**：
- 從 1-3 最高平台跳下不穿地、不被水平彈開。
- 點按跳 vs 按住跳高度差約 2 倍；原地跳 ≈ 4 格、助跑跳 ≈ 5 格。
- 殼速目測與玩家全速跑接近（原版殼略快於跑速）。

### Phase 2：碰撞系統重構（修 A-0 貼牆跳、A-1 穿牆的根本解）

涉及：`src/GameManager.cpp`（`CheckBlockCollision` 整段）、`include/Player.hpp`

1. **分軸解析**取代現有 Pass1/Pass2：
   - 先套用 X 位移 → 掃描重疊方塊，水平推回，記錄 `touchingLeft/Right`；
   - 再套用 Y 位移 → 掃描重疊方塊，向下落地（`onGround`）或向上撞頭（觸發 `OnHit`）。
   - 撞頭候選沿用「水平重疊最大者優先」的既有邏輯（`src/GameManager.cpp:858`）。
   - 刪除 `SURFACE_TOLERANCE` 容差猜測與 `standingOverlap` 補丁。
2. **新增 `ContactState` struct**（玩家碰撞狀態）：
   ```cpp
   struct ContactState {
       bool onGround = false;
       bool touchingLeft = false, touchingRight = false;
       Block* hitCeilingBlock = nullptr;  // 本幀頂到的方塊
       Block* standingOnBlock = nullptr;  // 腳下方塊（移動平台搭載用）
   };
   ```
   由碰撞解析每幀輸出；跳躍資格、進水管判定、`MovingPlatformBlock` 搭載
   全部改查此 struct，取代散落的 `m_OnGround` setter。
   **不**快取「可破壞方塊 / 可踩敵人」清單——可破壞性由 `Block::OnHit()` 當下決定、
   可踩性由相對運動方向當下決定，快取會有過期問題。
3. **HiddenBlock 單向實體**：只有「玩家上升且上一幀頭頂在磚底以下」時參與碰撞
   （在分軸解析的 Y 軸上升段特判 `Type::Hidden`）。
4. **水管輸入**：方向鍵改 `IsKeyPressed`（按住有效）；「向上進管」要求玩家
   `onGround == false` 且非跳躍上升幀，或將跳躍鍵與進管鍵分開。
5. **殼 vs 敵人碰撞 pass**：新增 enemy-vs-enemy 掃描——滑行殼碰到其他敵人 →
   對方翻面死亡 + combo 計分；一般行走敵人互撞 → 雙方 `ReverseDirection()`。
6. **頂磚互動**：磚塊 bounce 期間檢查站在磚上的敵人（殺死 +100）與道具（彈起）。
7. **Stomp 判定**改用 ContactState 與分軸結果，容差隨落速調整，
   消除高速下落「明明踩到卻判受傷」。

**驗收**：
- 貼牆下落按住方向 + 連打跳躍鍵，不會在牆面起跳。
- 在 JSON 放一顆 HiddenBlock：走路穿過去、跳躍從下方可頂出道具。
- 踢殼掃過一排 Goomba 全滅且分數翻倍；兩隻 Goomba 對撞會回頭。
- 頂磚可殺磚上的 Goomba。

### Phase 3：敵人與計分

涉及：`src/GameManager.cpp`、`src/Goomba.cpp`、`src/Koopa.cpp`、
`src/KoopaParatroopa.cpp`、`src/PiranhaPlant.cpp`、`src/GameSession.cpp`

1. 踩擊分數表 + 連踩 combo（落地前重置）；火球殺敵、踢殼計分。
2. Goomba 壓扁貼圖 + 0.4 秒延遲移除；翻面落下死亡動畫（共用於火球/殼/星星擊殺）。
3. 龜殼甦醒：縮殼 5 秒後開始抖動，再 2 秒復原行走（被踢則重置計時）。
4. 紅龜貼圖接上 variant；飛龜 VerticalPatrol 模式水平速度歸零。
5. 食人花：玩家距水管中心 < 約 2.5 格時不冒出；縮回時改為不可互動
   （`hidden` 狀態跳過碰撞與火球判定），z-order 用遮擋層或裁切解決「畫在水管前」。
6. 敲磚出金幣 +200 分、碎磚 +50 分。

**驗收**：連踩兩隻 Goomba 得 100+200；殼殺三隻顯示翻倍分數；
縮殼放著 7 秒會復活；站在水管旁食人花不冒頭。

### Phase 4：玩家規則與鏡頭

涉及：`src/Player.cpp`、`src/Camera.cpp`、`src/GameManager.cpp`、`src/MushroomItem.cpp`

1. 受傷規則改 SMB1：FIRE 受傷 → SMALL（直接）。
2. 變身/縮小動畫 + 全場凍結（新增 `Player::State::Transforming`，
   GameManager 在此狀態跳過世界更新，與現有 Dying 凍結同模式）。
3. 星星閃爍（sprite 色相循環或透明度閃爍），最後 2 秒變慢提示；
   星星期間踩敵改為觸碰即殺 + combo 計分。
4. 鏡頭只進不退（`m_X = max(m_X, target)`）+ 每幀呼叫 `ClampToCameraBounds`。
5. 移除 1/2/3 作弊鍵（或包進 `#ifdef DEBUG_CHEATS`）。
6. 蘑菇 `OnCollect`：只在 SMALL 時升級，否則 +1000 分不變身。

**驗收**：Fire 被打一下變小馬力歐並有閃爍無敵；鏡頭無法倒退；
玩家貼住畫面左緣不能再往左。

### Phase 5：遊戲流程

涉及：`src/GameManager.cpp`、`include/GameManager.hpp`、`src/GameSession.cpp`、
`Resources/data/*.json`

1. **關卡推進鏈**：過關後進入下一主關卡（1-1 → 1-2 → 1-3 → …），
   資料來源可在 JSON 加 `nextLevel` 欄位或在 GameManager 維護順序表；
   `GameSession::PlayerProgress.levelName` 已有欄位可存進度。
2. **時間系統**：改為每 0.4 秒扣 1 單位；過關結算「剩餘時間 × 50」
   逐格倒數加分（含 HUD 同步動畫）。
3. **中繼點**：JSON 加 `checkpoint` 物件；死亡時若已過中繼點，從中繼點重生
   （限同一條命週期內、未換關）。
4. **Time Up**：先讓玩家播死亡動畫（走既有 Dying 流程），再進扣命邏輯。
5. **GAME OVER 畫面**：lives 歸零 → 顯示 GAME OVER 約 3 秒 → 回標題。
6. 過關演出補完：旗子沿杆下降、馬力歐落地後跳下旗杆走向城堡門口消失。

**驗收**：1-1 過關自動進 1-2；時間 400 約 160 秒走完；
過半死亡從中繼點重生；沒命時看到 GAME OVER。

### Phase 6：表現層（HUD、音效、動畫）

涉及：`src/HUD.cpp`、`include/HUD.hpp`、各 sprite/animation 載入處

1. **HUD 重排成原版版面**（修 D 全項）：
   - 四欄位置改以 8px tile 網格計算（世界座標 × GAME_SCALE），
     上行標題與下行數值分成兩個獨立定位的 Text（不再用 `\n`）。
   - 金幣欄加閃爍金幣 sprite + `x00`。
   - 玩家名稱接 `GameSession::GetCurrentPlayerName()`。
   - WORLD 欄改由目前關卡名驅動（水管進子關卡時維持主關卡名，符合原版）。
2. **音效音樂**：用 PTSD 的 audio API（`Util::SFX` / `Util::BGM`）接上：
   跳躍、踩敵、頂磚、碎磚、金幣、吃菇、變身、死亡、過關、主題曲、地下曲、
   無敵曲、時間 < 100 加速警告。
3. 浮動分數文字（敵人/道具位置跳出 100/200/...，上飄淡出）。
4. 問號磚 3 幀閃爍動畫 + 被敲時彈跳（沿用 BrickBlock 的 bounce 實作）。
5. Goomba 走路 2 幀動畫（目前只有單張 `walk-1.png`）。
6. 暫停功能（Enter/P 鍵，凍結世界更新 + 顯示 PAUSE）。

**驗收**：HUD 與原版截圖逐欄比對位置一致；金幣圖示會閃爍；
全程有 BGM 與音效；暫停可用。

---

## 第三部分：附錄

### 附錄一：HiddenBlock 使用說明

目前**沒有任何關卡 JSON 用到隱藏磚塊**（`grep "Hidden" Resources/data/*.json` 無結果）。
程式已支援兩種寫法（`src/GameManager.cpp:537`）：

```json
{ "type": "HiddenBlock", "x": 1024, "y": 128, "itemType": "OneUp" }
```

`itemType` 留空預設出金幣。原版 1-1 的隱藏 1-UP 磚位於第一個坑後方、
中繼點前的空中（tile 座標約 x=64, y=高度 9，需依本專案座標系換算），
建議在 Phase 2 修好單向實體後補進 `1-1.json` / `1-1_ground_1.json`。

### 附錄二：原版 NES 物理數值換算表

換算規則：**世界座標單位 = NES 像素**（TILE_SIZE = 16），渲染才乘 GAME_SCALE = 3。
NES 以 px/frame@60fps 計，轉本專案 px/s 要 ×60，**不要再 ×3**。

| 項目 | NES (px/frame) | 本專案應為 (px/s) |
|---|---|---|
| 走路極速 | 1.5 | 90 |
| 跑步極速 | 2.5 | 150 |
| 跳躍初速（原地） | 4.0 | 240 |
| 上升重力（按住 A） | ≈ 0.125/f² | ≈ 450 px/s² |
| 下降/放開重力 | ≈ 0.4375/f² | ≈ 1575 px/s² |
| 終端落速 | 4.0 | 240 |
| 龜殼滑行 | 3.0 | 180 |
| Goomba / 蘑菇 | 0.6 | 36 |
| 火球 | 4.0 | 240 |

> 上升/下降重力的精確值依水平速度分組，原版有 3 組參數；
> 實作時以「點按跳 ≈ 1 格、按住原地跳 ≈ 4 格、全速跑跳 ≈ 5 格」回歸驗證即可。

### 附錄三：問題編號 ↔ Phase 對照

| Phase | 涵蓋問題 |
|---|---|
| 1 物理底層 | A-1, B-13, B-14, B-15 |
| 2 碰撞重構 | A-0, A-2, A-4, A-5, B-18, B-19（+ A-1 收尾）|
| 3 敵人計分 | A-6, A-7, A-8, A-9, B-16, B-17 |
| 4 玩家鏡頭 | A-3, A-10, A-11, A-12, C-20, C-21 |
| 5 遊戲流程 | C-22, C-23, C-24, C-25, C-27 |
| 6 表現層 | B-16（浮動分數）, C-26, D 全項 |
