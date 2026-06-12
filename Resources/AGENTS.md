# Resources Agent Notes

這份文件描述 runtime 關卡資料、asset path 與 JSON 格式。修改 `Resources/data/` 或 `Resources/Asset/` 時優先讀這份。

## 目錄責任

- `Resources/data/`：目前實際載入的 JSON 關卡。
- `Resources/Asset/`：實際使用的圖像、字體、音效等資產。
- `Resources/Asset/player/Effects/`：由既有 Mario 動作圖調色產生的受傷與星星無敵配色，目錄結構與原始 player sprite 相同。
- `Resources/Asset/enemy/Koopa/<theme>/`：Koopa runtime 素材；`normal/reverse` 為綠/地下配色，`red/red_reverse` 含紅龜 walk、fly、shell 與 shell_revive。
- CMake 的 `RESOURCE_DIR` 指向 `Resources/`；runtime 資料與資產都集中在這裡。
- `ExportedSprites/` 是匯出的 sprite 素材/參考，不是目前程式直接使用的主要 asset root。

## Asset Path

- 程式用 `MakeAssetPath()` 將相對路徑接到 `RESOURCE_DIR/Asset`。
- JSON 的 `backgroundImage` 使用相對 `Resources/Asset/` 的路徑，例如 `level_image/1-1/ground.png`。
- 字體目前使用 `Resources/Asset/font/Super Mario Bros. NES.ttf`。

## JSON Level Format

詳細格式看頂層 `JSON_LEVEL_TUTORIAL.md`。核心欄位：

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
- `HiddenBlock`
- `MultiCoinBlock`
- `Coin`
- `CollectibleCoin`

支援 enemy types：

- `Goomba`
- `Koopa`
- `KoopaParatroopa`
- `PiranhaPlant`

常見 item types：

- `Coin`
- `PowerUp`
- `Mushroom`
- `FireFlower`
- `OneUp`
- `Star`
- `None`

`PowerUp` 在 `GameManager::SpawnItem()` 內依玩家形態決定：SMALL 產生 mushroom，SUPER/FIRE 產生 fire flower。

## 重要物件格式細節

- `PipeBlock`
  - JSON 用一格管口 anchor + `segments` 表示長度；C++ 依方向自動計算碰撞尺寸。
  - 支援 `opening`：`up`、`down`、`left`、`right`。
  - 可 enterable，並用 `targetLevel` 或 `exitToLevel` 切關。
  - `targetSpawn` 可覆蓋切關後玩家位置。
- `FlagBlock`
  - JSON y 是旗杆底部；內部用 `FLAG_POLE_TILES` 往上推算碰撞高度。
- `MovingPlatformBlock`
  - JSON 用一格起點 anchor + `segments` 表示平台長度，`moveTiles` 表示移動距離。
  - 支援 `moveMode`：`oscillate`、`verticalWrap`。
- `MultiCoinBlock`
  - 可用 `coinCount` 控制可敲出 coin 數量，預設 10。

## 新增關卡

1. 在 `Resources/data/` 新增 `<name>.json`。
2. 背景圖放在 `Resources/Asset/level_image/...`。
3. JSON 的 `backgroundImage` 用相對 `Resources/Asset/` 的路徑，例如 `level_image/1-1/ground.png`。
4. 若要從水管切換，`targetLevel` 可寫不含 `.json` 的名稱。
5. 確認 `levelWidth`、`levelHeight` 與背景圖/碰撞物件一致。
6. 如果新增 schema、object type、enemy type 或 item type，同步更新 `JSON_LEVEL_TUTORIAL.md`、`Resources/AGENTS.md`，以及需要的 `src/AGENTS.md` / `include/AGENTS.md`。
