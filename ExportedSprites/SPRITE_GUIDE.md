# ExportedSprites 素材說明

這份文件說明 `ExportedSprites/` 內從 Unity sprite sheet 切出的 PNG 素材用途。素材來自 `ExportedSprites/manifest.json`，同一張 sprite 只在總表解釋一次；如果同一素材被多個關卡使用，會列出所有出現關卡。

## 結構總覽

| 路徑 | 說明 |
| --- | --- |
| `ExportedSprites/1-1/` | World 1-1 實際引用到的素材，依用途分類。 |
| `ExportedSprites/1-2/` | World 1-2 實際引用到的素材，依用途分類。 |
| `ExportedSprites/1-3/` | World 1-3 實際引用到的素材，依用途分類。 |
| `ExportedSprites/title/` | `title.png` 切出的標題畫面素材。 |
| `ExportedSprites/manifest.json` | 匯出清單，記錄來源 atlas、Unity GUID/fileID、rect、分類與輸出路徑。 |

## 統計

- Gameplay unique sprites: 125
- Title sprites: 2
- Exported PNG total: 248

## 分類說明

| 分類 | 中文名稱 | 用途 |
| --- | --- | --- |
| `player` | 玩家角色 | Mario 各狀態的站立、跑步、跳躍、攀爬、蹲下、死亡，以及火球相關動畫幀。 |
| `enemies` | 敵人 | 敵人行走、死亡、進殼、復活、飛行與攻擊動畫幀。 |
| `blocks` | 方塊/磚塊 | 問號磚、磚塊、用後方塊與磚塊碎片。 |
| `collectibles` | 道具/可收集物 | 金幣、蘑菇、火花、星星等道具與閃爍/旋轉動畫幀。 |
| `ground` | 地形 | 主要地板 tile、樹頂平台與可站立地形。 |
| `pipes` | 管子 | 管子頂部、管身、側向管道組件。 |
| `platforms` | 平台 | 一般平台、移動平台、橋面或城堡平台素材。 |
| `scenery` | 背景裝飾 | 雲、草叢、山丘等背景裝飾，不直接提供 gameplay collision。 |
| `finish` | 終點物件 | 旗桿、旗幟、城堡與過關終點相關素材。 |
| `ui` | 介面 | HUD 金幣圖示等介面素材。 |
| `misc` | 其他 | 少量無法歸入上述分類的輔助圖塊。 |
| `title` | 標題畫面 | 標題畫面 logo 與選單游標。 |

## 去重素材總表

### 玩家角色 (`player`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `big_mario_brake` | `16x32` | 大 Mario 煞車/急停幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_brake.png` |
| `big_mario_climb_1` | `16x32` | 大 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_climb_1.png` |
| `big_mario_climb_2` | `16x32` | 大 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_climb_2.png` |
| `big_mario_crouch` | `16x32` | 大 Mario 蹲下幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_crouch.png` |
| `big_mario_jump` | `16x32` | 大 Mario 跳躍幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_jump.png` |
| `big_mario_stand` | `16x32` | 大 Mario 站立待機幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_stand.png` |
| `big_mario_walk_1` | `16x32` | 大 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_walk_1.png` |
| `big_mario_walk_2` | `16x32` | 大 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_walk_2.png` |
| `big_mario_walk_3` | `16x32` | 大 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__big_mario_walk_3.png` |
| `fire_mario_brake` | `16x32` | 火焰 Mario 煞車/急停幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_brake.png` |
| `fire_mario_climb_1` | `16x32` | 火焰 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_climb_1.png` |
| `fire_mario_climb_2` | `16x32` | 火焰 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_climb_2.png` |
| `fire_mario_crouch` | `16x32` | 火焰 Mario 蹲下幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_crouch.png` |
| `fire_mario_fire` | `16x30` | 火焰 Mario 發射火球的動作幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/mario__fire_mario_fire.png` |
| `fire_mario_jump` | `16x32` | 火焰 Mario 跳躍幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_jump.png` |
| `fire_mario_stand` | `16x32` | 火焰 Mario 站立待機幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_stand.png` |
| `fire_mario_walk_1` | `16x32` | 火焰 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_walk_1.png` |
| `fire_mario_walk_2` | `16x32` | 火焰 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_walk_2.png` |
| `fire_mario_walk_3` | `16x32` | 火焰 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__fire_mario_walk_3.png` |
| `mario_die` | `16x32` | Mario 死亡動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__mario_die.png` |
| `mario_fire_1` | `8x8` | Mario 火球飛行旋轉動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_1.png` |
| `mario_fire_2` | `8x8` | Mario 火球飛行旋轉動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_2.png` |
| `mario_fire_3` | `8x8` | Mario 火球飛行旋轉動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_3.png` |
| `mario_fire_4` | `8x8` | Mario 火球飛行旋轉動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_4.png` |
| `mario_fire_hit_1` | `8x8` | Mario 火球命中/爆裂動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_hit_1.png` |
| `mario_fire_hit_2` | `12x14` | Mario 火球命中/爆裂動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_hit_2.png` |
| `mario_fire_hit_3` | `16x16` | Mario 火球命中/爆裂動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/misc_3__mario_fire_hit_3.png` |
| `small_mario_brake` | `16x32` | 小 Mario 煞車/急停幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_brake.png` |
| `small_mario_climb_1` | `16x32` | 小 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_climb_1.png` |
| `small_mario_climb_2` | `16x32` | 小 Mario 爬藤/攀爬動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_climb_2.png` |
| `small_mario_jump` | `16x32` | 小 Mario 跳躍幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_jump.png` |
| `small_mario_stand` | `16x32` | 小 Mario 站立待機幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_stand.png` |
| `small_mario_transform` | `16x32` | 小 Mario 變身過渡幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_transform.png` |
| `small_mario_walk_1` | `16x32` | 小 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_walk_1.png` |
| `small_mario_walk_2` | `16x32` | 小 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_walk_2.png` |
| `small_mario_walk_3` | `16x32` | 小 Mario 跑步動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/player/characters__small_mario_walk_3.png` |

### 敵人 (`enemies`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `brown_goomba_1` | `16x16` | Goomba 行走動畫幀。 | `1-1`, `1-3` | `ExportedSprites/1-1/enemies/enemies__brown_goomba_1.png` |
| `brown_goomba_2` | `16x16` | Goomba 行走動畫幀。 | `1-1`, `1-3` | `ExportedSprites/1-1/enemies/enemies__brown_goomba_2.png` |
| `brown_goomba_die` | `16x16` | Goomba 被踩扁/死亡狀態幀。 | `1-1`, `1-3` | `ExportedSprites/1-1/enemies/enemies__brown_goomba_die.png` |
| `green_goomba_1` | `16x16` | Goomba 行走動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__green_goomba_1.png` |
| `green_goomba_2` | `16x16` | Goomba 行走動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__green_goomba_2.png` |
| `green_goomba_die` | `16x17` | Goomba 被踩扁/死亡狀態幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__green_goomba_die.png` |
| `green_koopa_1` | `16x23` | Koopa 行走動畫幀。 | `1-1` | `ExportedSprites/1-1/enemies/enemies__green_koopa_1.png` |
| `green_koopa_2` | `16x24` | Koopa 行走動畫幀。 | `1-1` | `ExportedSprites/1-1/enemies/enemies__green_koopa_2.png` |
| `green_koopa_shell` | `16x14` | Koopa 進殼後的殼狀態幀。 | `1-1` | `ExportedSprites/1-1/enemies/enemies__green_koopa_shell.png` |
| `green_koopa_shell_revive` | `16x15` | Koopa 殼即將復活的動畫幀。 | `1-1` | `ExportedSprites/1-1/enemies/enemies__green_koopa_shell_revive.png` |
| `red_koopa_1` | `16x23` | Koopa 行走動畫幀。 | `1-2`, `1-3` | `ExportedSprites/1-2/enemies/enemies__red_koopa_1.png` |
| `red_koopa_2` | `16x24` | Koopa 行走動畫幀。 | `1-2`, `1-3` | `ExportedSprites/1-2/enemies/enemies__red_koopa_2.png` |
| `red_koopa_shell` | `16x14` | Koopa 進殼後的殼狀態幀。 | `1-2`, `1-3` | `ExportedSprites/1-2/enemies/enemies__red_koopa_shell.png` |
| `red_koopa_shell_revive` | `16x15` | Koopa 殼即將復活的動畫幀。 | `1-2`, `1-3` | `ExportedSprites/1-2/enemies/enemies__red_koopa_shell_revive.png` |
| `red_wing_koopa_1` | `16x23` | 有翅 Koopa 飛行/行走動畫幀。 | `1-3` | `ExportedSprites/1-3/enemies/enemies__red_wing_koopa_1.png` |
| `red_wing_koopa_2` | `16x24` | 有翅 Koopa 飛行/行走動畫幀。 | `1-3` | `ExportedSprites/1-3/enemies/enemies__red_wing_koopa_2.png` |
| `teal_koopa_1` | `16x23` | Koopa 行走動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_koopa_1.png` |
| `teal_koopa_2` | `16x24` | Koopa 行走動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_koopa_2.png` |
| `teal_koopa_shell` | `16x14` | Koopa 進殼後的殼狀態幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_koopa_shell.png` |
| `teal_koopa_shell_revive` | `16x15` | Koopa 殼即將復活的動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_koopa_shell_revive.png` |
| `teal_piranha_1` | `16x24` | 食人花開合動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_piranha_1.png` |
| `teal_piranha_2` | `16x23` | 食人花開合動畫幀。 | `1-2` | `ExportedSprites/1-2/enemies/enemies__teal_piranha_2.png` |

### 方塊/磚塊 (`blocks`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `block_brown` | `16x16` | 棕色用後/失效方塊，常作問號磚被頂完後的狀態。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/blocks/misc_3__block_brown.png` |
| `block_brown_green_tinge` | `16x16` | 靜態或用後狀態方塊。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__block_brown_green_tinge.png` |
| `block_green` | `16x16` | 靜態或用後狀態方塊。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__block_green.png` |
| `brick_brown1` | `16x16` | 可見磚塊，本專案可作一般磚或藏道具磚。 | `1-1` | `ExportedSprites/1-1/blocks/misc_3__brick_brown1.png` |
| `brick_brown_piece` | `8x8` | 磚塊被打碎時飛出的碎片。 | `1-1` | `ExportedSprites/1-1/blocks/misc_3__brick_brown_piece.png` |
| `brick_green2` | `16x17` | 可見磚塊，本專案可作一般磚或藏道具磚。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__brick_green2.png` |
| `brick_green_piece` | `8x8` | 磚塊被打碎時飛出的碎片。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__brick_green_piece.png` |
| `question_block1_1` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-1`, `1-3` | `ExportedSprites/1-1/blocks/misc_3__question_block1_1.png` |
| `question_block1_2` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-1`, `1-3` | `ExportedSprites/1-1/blocks/misc_3__question_block1_2.png` |
| `question_block1_3` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-1`, `1-3` | `ExportedSprites/1-1/blocks/misc_3__question_block1_3.png` |
| `question_block2_1` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__question_block2_1.png` |
| `question_block2_2` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__question_block2_2.png` |
| `question_block2_3` | `16x16` | 問號磚動畫幀，可頂出金幣或道具。 | `1-2` | `ExportedSprites/1-2/blocks/misc_3__question_block2_3.png` |

### 道具/可收集物 (`collectibles`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `big_mushroom` | `16x16` | 蘑菇類道具，包含變大蘑菇或 1UP。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/collectibles/misc_3__big_mushroom.png` |
| `coin1_1` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-3` | `ExportedSprites/1-3/collectibles/misc_3__coin1_1.png` |
| `coin1_2` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-3` | `ExportedSprites/1-3/collectibles/misc_3__coin1_2.png` |
| `coin1_3` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-3` | `ExportedSprites/1-3/collectibles/misc_3__coin1_3.png` |
| `coin2_1` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-2` | `ExportedSprites/1-2/collectibles/misc_3__coin2_1.png` |
| `coin2_2` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-2` | `ExportedSprites/1-2/collectibles/misc_3__coin2_2.png` |
| `coin2_3` | `10x14` | 金幣閃爍/旋轉動畫幀。 | `1-2` | `ExportedSprites/1-2/collectibles/misc_3__coin2_3.png` |
| `coin_flashing_1` | `8x14` | 金幣閃爍/旋轉動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__coin_flashing_1.png` |
| `coin_flashing_2` | `8x14` | 金幣閃爍/旋轉動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__coin_flashing_2.png` |
| `coin_flashing_3` | `8x14` | 金幣閃爍/旋轉動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__coin_flashing_3.png` |
| `coin_flashing_4` | `8x14` | 金幣閃爍/旋轉動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__coin_flashing_4.png` |
| `fireflower_1` | `16x16` | 火花道具閃爍動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/collectibles/misc_3__fireflower_1.png` |
| `fireflower_2` | `16x16` | 火花道具閃爍動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/collectibles/misc_3__fireflower_2.png` |
| `fireflower_3` | `16x16` | 火花道具閃爍動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/collectibles/misc_3__fireflower_3.png` |
| `fireflower_4` | `16x16` | 火花道具閃爍動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/collectibles/misc_3__fireflower_4.png` |
| `oneup_mushroom_dark` | `16x16` | 蘑菇類道具，包含變大蘑菇或 1UP。 | `1-2` | `ExportedSprites/1-2/collectibles/misc_3__oneup_mushroom_dark.png` |
| `oneup_mushroom_light` | `16x16` | 蘑菇類道具，包含變大蘑菇或 1UP。 | `1-1` | `ExportedSprites/1-1/collectibles/misc_3__oneup_mushroom_light.png` |
| `starman_1` | `14x16` | 無敵星星閃爍動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__starman_1.png` |
| `starman_2` | `14x16` | 無敵星星閃爍動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__starman_2.png` |
| `starman_3` | `14x16` | 無敵星星閃爍動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__starman_3.png` |
| `starman_4` | `14x16` | 無敵星星閃爍動畫幀。 | `1-1`, `1-2` | `ExportedSprites/1-1/collectibles/misc_3__starman_4.png` |

### 地形 (`ground`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `ground1_brown` | `16x16` | 主要地面磚 tile，用來拼地板或實心平台。 | `1-1`, `1-3` | `ExportedSprites/1-1/ground/misc_3__ground1_brown.png` |
| `ground1_green` | `16x16` | 主要地面磚 tile，用來拼地板或實心平台。 | `1-2` | `ExportedSprites/1-2/ground/misc_3__ground1_green.png` |
| `tree_top_left` | `16x16` | 樹頂平台左端。 | `1-3` | `ExportedSprites/1-3/ground/misc_3__tree_top_left.png` |
| `tree_top_mid` | `16x16` | 樹頂平台中段，可重複拼接。 | `1-3` | `ExportedSprites/1-3/ground/misc_3__tree_top_mid.png` |
| `tree_top_right` | `16x16` | 樹頂平台右端。 | `1-3` | `ExportedSprites/1-3/ground/misc_3__tree_top_right.png` |

### 管子 (`pipes`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `pipe_green_body` | `28x16` | 直立管子的管身，可垂直堆疊。 | `1-1`, `1-2` | `ExportedSprites/1-1/pipes/misc_3__pipe_green_body.png` |
| `pipe_green_mid` | `34x32` | 側向管子的中段。 | `1-2` | `ExportedSprites/1-2/pipes/misc_3__pipe_green_mid.png` |
| `pipe_green_top_side` | `32x32` | 側向開口管子的端口。 | `1-2` | `ExportedSprites/1-2/pipes/misc_3__pipe_green_top_side.png` |
| `pipe_green_top_up` | `32x32` | 向上開口管子的頂部。 | `1-1`, `1-2` | `ExportedSprites/1-1/pipes/misc_3__pipe_green_top_up.png` |

### 平台 (`platforms`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `marble_brown` | `16x16` | 城堡/平台用磚面或橋面 tile。 | `1-1`, `1-3` | `ExportedSprites/1-1/platforms/misc_3__marble_brown.png` |
| `marble_green` | `16x16` | 城堡/平台用磚面或橋面 tile。 | `1-2` | `ExportedSprites/1-2/platforms/misc_3__marble_green.png` |
| `moving_platform_6` | `48x8` | 移動平台的可見平台片。 | `1-2`, `1-3` | `ExportedSprites/1-2/platforms/misc_3__moving_platform_6.png` |

### 背景裝飾 (`scenery`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `bg_bush_1` | `32x16` | 背景草叢裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_bush_1.png` |
| `bg_bush_2` | `48x16` | 背景草叢裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_bush_2.png` |
| `bg_bush_3` | `64x16` | 背景草叢裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_bush_3.png` |
| `bg_cloud_blue_1` | `32x24` | 背景雲朵裝飾，不參與碰撞。 | `1-1`, `1-3` | `ExportedSprites/1-1/scenery/misc_3__bg_cloud_blue_1.png` |
| `bg_cloud_blue_2` | `48x24` | 背景雲朵裝飾，不參與碰撞。 | `1-1`, `1-3` | `ExportedSprites/1-1/scenery/misc_3__bg_cloud_blue_2.png` |
| `bg_cloud_blue_3` | `64x24` | 背景雲朵裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_cloud_blue_3.png` |
| `bg_mountain_big` | `80x35` | 背景山丘裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_mountain_big.png` |
| `bg_mountain_small` | `48x19` | 背景山丘裝飾，不參與碰撞。 | `1-1` | `ExportedSprites/1-1/scenery/misc_3__bg_mountain_small.png` |

### 終點物件 (`finish`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `castle_big_left` | `40x176` | 關卡終點城堡或城堡旗幟組件。 | `1-3` | `ExportedSprites/1-3/finish/misc_3__castle_big_left.png` |
| `castle_big_right` | `109x176` | 關卡終點城堡或城堡旗幟組件。 | `1-3` | `ExportedSprites/1-3/finish/misc_3__castle_big_right.png` |
| `castle_flag` | `13x14` | 關卡終點城堡或城堡旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__castle_flag.png` |
| `castle_small_left` | `40x80` | 關卡終點城堡或城堡旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__castle_small_left.png` |
| `castle_small_right` | `40x80` | 關卡終點城堡或城堡旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__castle_small_right.png` |
| `castle_wall` | `16x96` | 關卡終點城堡或城堡旗幟組件。 | `1-3` | `ExportedSprites/1-3/finish/misc_3__castle_wall.png` |
| `goal_flag_green` | `16x16` | 終點旗桿/旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__goal_flag_green.png` |
| `goal_pole_green` | `2x127` | 終點旗桿/旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__goal_pole_green.png` |
| `goal_top_green` | `8x8` | 終點旗桿/旗幟組件。 | `1-1`, `1-3` | `ExportedSprites/1-1/finish/misc_3__goal_top_green.png` |

### 介面 (`ui`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `hud_coin1_1` | `5x8` | HUD 金幣圖示動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/ui/misc_3__hud_coin1_1.png` |
| `hud_coin1_2` | `5x8` | HUD 金幣圖示動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/ui/misc_3__hud_coin1_2.png` |
| `hud_coin1_3` | `5x8` | HUD 金幣圖示動畫幀。 | `1-1`, `1-2`, `1-3` | `ExportedSprites/1-1/ui/misc_3__hud_coin1_3.png` |

### 其他 (`misc`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `tree_chunk` | `16x16` | 樹幹/樹平台支撐塊。 | `1-3` | `ExportedSprites/1-3/misc/misc_3__tree_chunk.png` |

### 標題畫面 (`title`)

| Sprite | 尺寸 | 用途 | 出現關卡 | 檔案範例 |
| --- | --- | --- | --- | --- |
| `cursor` | `8x8` | 標題/選單游標，用來指示目前選項。 | `title` | `ExportedSprites/title/cursor.png` |
| `logo` | `176x88` | 標題畫面的 Super Mario Bros logo。 | `title` | `ExportedSprites/title/logo.png` |

## 每關索引

### 1-1

- **玩家角色 (`player`)**: `big_mario_brake`, `big_mario_climb_1`, `big_mario_climb_2`, `big_mario_crouch`, `big_mario_jump`, `big_mario_stand`, `big_mario_walk_1`, `big_mario_walk_2`, `big_mario_walk_3`, `fire_mario_brake`, `fire_mario_climb_1`, `fire_mario_climb_2`, `fire_mario_crouch`, `fire_mario_fire`, `fire_mario_jump`, `fire_mario_stand`, `fire_mario_walk_1`, `fire_mario_walk_2`, `fire_mario_walk_3`, `mario_die`, `mario_fire_1`, `mario_fire_2`, `mario_fire_3`, `mario_fire_4`, `mario_fire_hit_1`, `mario_fire_hit_2`, `mario_fire_hit_3`, `small_mario_brake`, `small_mario_climb_1`, `small_mario_climb_2`, `small_mario_jump`, `small_mario_stand`, `small_mario_transform`, `small_mario_walk_1`, `small_mario_walk_2`, `small_mario_walk_3`
- **敵人 (`enemies`)**: `brown_goomba_1`, `brown_goomba_2`, `brown_goomba_die`, `green_koopa_1`, `green_koopa_2`, `green_koopa_shell`, `green_koopa_shell_revive`
- **方塊/磚塊 (`blocks`)**: `block_brown`, `brick_brown1`, `brick_brown_piece`, `question_block1_1`, `question_block1_2`, `question_block1_3`
- **道具/可收集物 (`collectibles`)**: `big_mushroom`, `coin_flashing_1`, `coin_flashing_2`, `coin_flashing_3`, `coin_flashing_4`, `fireflower_1`, `fireflower_2`, `fireflower_3`, `fireflower_4`, `oneup_mushroom_light`, `starman_1`, `starman_2`, `starman_3`, `starman_4`
- **地形 (`ground`)**: `ground1_brown`
- **管子 (`pipes`)**: `pipe_green_body`, `pipe_green_top_up`
- **平台 (`platforms`)**: `marble_brown`
- **背景裝飾 (`scenery`)**: `bg_bush_1`, `bg_bush_2`, `bg_bush_3`, `bg_cloud_blue_1`, `bg_cloud_blue_2`, `bg_cloud_blue_3`, `bg_mountain_big`, `bg_mountain_small`
- **終點物件 (`finish`)**: `castle_flag`, `castle_small_left`, `castle_small_right`, `goal_flag_green`, `goal_pole_green`, `goal_top_green`
- **介面 (`ui`)**: `hud_coin1_1`, `hud_coin1_2`, `hud_coin1_3`

### 1-2

- **玩家角色 (`player`)**: `big_mario_brake`, `big_mario_climb_1`, `big_mario_climb_2`, `big_mario_crouch`, `big_mario_jump`, `big_mario_stand`, `big_mario_walk_1`, `big_mario_walk_2`, `big_mario_walk_3`, `fire_mario_brake`, `fire_mario_climb_1`, `fire_mario_climb_2`, `fire_mario_crouch`, `fire_mario_fire`, `fire_mario_jump`, `fire_mario_stand`, `fire_mario_walk_1`, `fire_mario_walk_2`, `fire_mario_walk_3`, `mario_die`, `mario_fire_1`, `mario_fire_2`, `mario_fire_3`, `mario_fire_4`, `mario_fire_hit_1`, `mario_fire_hit_2`, `mario_fire_hit_3`, `small_mario_brake`, `small_mario_climb_1`, `small_mario_climb_2`, `small_mario_jump`, `small_mario_stand`, `small_mario_transform`, `small_mario_walk_1`, `small_mario_walk_2`, `small_mario_walk_3`
- **敵人 (`enemies`)**: `green_goomba_1`, `green_goomba_2`, `green_goomba_die`, `red_koopa_1`, `red_koopa_2`, `red_koopa_shell`, `red_koopa_shell_revive`, `teal_koopa_1`, `teal_koopa_2`, `teal_koopa_shell`, `teal_koopa_shell_revive`, `teal_piranha_1`, `teal_piranha_2`
- **方塊/磚塊 (`blocks`)**: `block_brown`, `block_brown_green_tinge`, `block_green`, `brick_green2`, `brick_green_piece`, `question_block2_1`, `question_block2_2`, `question_block2_3`
- **道具/可收集物 (`collectibles`)**: `big_mushroom`, `coin2_1`, `coin2_2`, `coin2_3`, `coin_flashing_1`, `coin_flashing_2`, `coin_flashing_3`, `coin_flashing_4`, `fireflower_1`, `fireflower_2`, `fireflower_3`, `fireflower_4`, `oneup_mushroom_dark`, `starman_1`, `starman_2`, `starman_3`, `starman_4`
- **地形 (`ground`)**: `ground1_green`
- **管子 (`pipes`)**: `pipe_green_body`, `pipe_green_mid`, `pipe_green_top_side`, `pipe_green_top_up`
- **平台 (`platforms`)**: `marble_green`, `moving_platform_6`
- **介面 (`ui`)**: `hud_coin1_1`, `hud_coin1_2`, `hud_coin1_3`

### 1-3

- **玩家角色 (`player`)**: `big_mario_brake`, `big_mario_climb_1`, `big_mario_climb_2`, `big_mario_crouch`, `big_mario_jump`, `big_mario_stand`, `big_mario_walk_1`, `big_mario_walk_2`, `big_mario_walk_3`, `fire_mario_brake`, `fire_mario_climb_1`, `fire_mario_climb_2`, `fire_mario_crouch`, `fire_mario_fire`, `fire_mario_jump`, `fire_mario_stand`, `fire_mario_walk_1`, `fire_mario_walk_2`, `fire_mario_walk_3`, `mario_die`, `mario_fire_1`, `mario_fire_2`, `mario_fire_3`, `mario_fire_4`, `mario_fire_hit_1`, `mario_fire_hit_2`, `mario_fire_hit_3`, `small_mario_brake`, `small_mario_climb_1`, `small_mario_climb_2`, `small_mario_jump`, `small_mario_stand`, `small_mario_transform`, `small_mario_walk_1`, `small_mario_walk_2`, `small_mario_walk_3`
- **敵人 (`enemies`)**: `brown_goomba_1`, `brown_goomba_2`, `brown_goomba_die`, `red_koopa_1`, `red_koopa_2`, `red_koopa_shell`, `red_koopa_shell_revive`, `red_wing_koopa_1`, `red_wing_koopa_2`
- **方塊/磚塊 (`blocks`)**: `block_brown`, `question_block1_1`, `question_block1_2`, `question_block1_3`
- **道具/可收集物 (`collectibles`)**: `big_mushroom`, `coin1_1`, `coin1_2`, `coin1_3`, `fireflower_1`, `fireflower_2`, `fireflower_3`, `fireflower_4`
- **地形 (`ground`)**: `ground1_brown`, `tree_top_left`, `tree_top_mid`, `tree_top_right`
- **平台 (`platforms`)**: `marble_brown`, `moving_platform_6`
- **背景裝飾 (`scenery`)**: `bg_cloud_blue_1`, `bg_cloud_blue_2`
- **終點物件 (`finish`)**: `castle_big_left`, `castle_big_right`, `castle_flag`, `castle_small_left`, `castle_small_right`, `castle_wall`, `goal_flag_green`, `goal_pole_green`, `goal_top_green`
- **介面 (`ui`)**: `hud_coin1_1`, `hud_coin1_2`, `hud_coin1_3`
- **其他 (`misc`)**: `tree_chunk`

### title

- **標題畫面 (`title`)**: `cursor`, `logo`

## 讀法備註

- 檔名格式為 `來源圖集__sprite_name.png`，例如 `misc_3__pipe_green_top_up.png` 表示它來自 `misc-3.gif` 的 `pipe_green_top_up` 切片。
- `rect` 尺寸是原始 1x sprite 尺寸；遊戲顯示時可能再依引擎設定放大。
- `ExportedSprites` 是從 Unity 素材匯出的結果，不會反向影響 `Assets/` 內原始 prefab、scene 或 `.meta`。
- 每個用途是依分類、檔名與來源 animation/prefab context 推斷，目標是幫助快速選素材與建立 JSON 關卡資料。
