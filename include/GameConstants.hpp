#ifndef GAME_CONSTANTS_HPP
#define GAME_CONSTANTS_HPP

/*
 * ╔══════════════════════════════════════════════════════════╗
 * ║              Mario 專案 — 全域常數定義                   ║
 * ║  所有數值相關的設計決策都記錄在這裡                        ║
 * ║  需要用到數字的地方，請 include 這個檔案並使用常數名稱      ║
 * ╚══════════════════════════════════════════════════════════╝
 */

// ─── 地圖 / 圖塊 ──────────────────────────────────────────────
/*
 * 地圖的每個圖塊（Tile）邊長 16 像素
 * 所有地圖物件的位置、大小都以這個為單位設計
 * 例如：一格磚塊 = 16x16，Mario 高度 = 2 格 = 32 像素
 */
constexpr float TILE_SIZE = 16.0f;

/*
 * FLAG_POLE_TILES：終點旗杆的高度（格數）
 *
 * JSON 只記錄旗杆頂端的座標，程式自動往下延伸這麼多格。
 * 改這個數字就能調整旗杆高度，不需要動 JSON。
 */
constexpr int FLAG_POLE_TILES = 10;

/*
 * 過關旗杆 → 城堡 的演出參數
 *
 * 背景圖把「旗杆 + 杆頂球 + 城堡」都畫死了，程式只負責會動的東西：
 * 旗子下滑、馬力歐滑杆、走進城門、城堡升小旗。
 *
 * POLE_SLIDE_SPEED：馬力歐與旗子沿杆下滑的速度（兩者共用，確保同步）。
 * DEFAULT_LEVEL_CLEAR_WALK_TILES：碰旗後從旗杆走到城堡門中心的預設距離。
 * DEFAULT_CASTLE_FLAG_BASE_TILES ：地面到城堡小旗底部/塔頂的預設高度。
 * CASTLE_FLAG_SPEED              ：城堡頂小旗升起動畫速度。
 */
constexpr float POLE_SLIDE_SPEED = 150.0f; // 滑杆速度（世界像素/秒）
constexpr float DEFAULT_LEVEL_CLEAR_WALK_TILES = 6.0f;
constexpr float DEFAULT_CASTLE_FLAG_BASE_TILES = 4.0f;
constexpr float CASTLE_FLAG_SPEED = 32.0f; // 小旗升起速度（世界像素/秒）

/*
 * GAME_SCALE：全域畫面縮放比例
 *
 * NES 原始高度是 240px。
 * PTSD 視窗高度預設為 720px。
 * 統一將所有渲染物件放大 3.0 倍 (240 * 3 = 720)，確保畫面完美填滿。
 */
constexpr float GAME_SCALE = 3.0f;

// ─── 玩家物理：垂直（世界像素/秒；未乘 GAME_SCALE）────────────────
constexpr float PLAYER_JUMP_VELOCITY      = 255.0f;  // 腳峰值約 72px ≈ 4.5 tiles，過 4 tile 牆餘裕約 8px（如 1-2 地下）
constexpr float PLAYER_JUMP_RUN_BONUS     =   0.0f;  // 跑跳不再加高，靠水平跑速拉出原版距離
constexpr float PLAYER_RUN_JUMP_THRESHOLD =  90.0f;  // |vx| 超過走路極速才吃 bonus
constexpr float GRAVITY_RISE              = 450.0f;  // 上升中且按住跳：弱重力（= SMB1 走路 $20 → 450）
constexpr float GRAVITY_APEX              = 350.0f;  // 頂點附近短暫弱重力，讓第 4 格附近多停一下
constexpr float PLAYER_APEX_HANG_SPEED    =  45.0f;  // |vy| 低於此值時進入頂點滯空區
constexpr float GRAVITY_FALL              = 1550.0f; // 放開跳 或 vy>=0：強重力（≈ SMB1 $70 → 1575，下降俐落不飄）
constexpr float MAX_FALL_SPEED            = 290.0f;  // 終端速度：≈ SMB1 ~4.5px/f（原 400 偏高，失重感太強）

// ─── 玩家物理：水平 ───────────────────────────────────────────────────────
constexpr float PLAYER_MAX_WALK_SPEED     =  90.0f;  // 走路最高速（NES ≈ 93.75）
constexpr float PLAYER_MAX_RUN_SPEED      = 145.0f;  // 奔跑最高速；配合 4.25 格跳高後跑跳距離 ≈ SMB1
constexpr float PLAYER_ACCELERATION       = 200.0f;  // 水平加速度（沿用現值）
constexpr float PLAYER_RUN_ACCEL_MULT     =   1.5f;  // 地面奔跑加速度倍率（沿用現有 *1.5 邏輯）
constexpr float PLAYER_SKID_DECEL         = 400.0f;  // 轉向煞車（打滑，沿用現值）
constexpr float PLAYER_FRICTION           = 200.0f;  // 放開方向鍵摩擦（沿用現值）

// ─── 其他 ─────────────────────────────────────────────────────────────────
constexpr float PLAYER_DEATH_BOUNCE       = 280.0f;  // 死亡上彈初速；約 3.5 tiles，接近原版短彈起
constexpr float ENEMY_GRAVITY             = 700.0f;  // 敵人/死亡動畫重力（沿用 Character 現值）
constexpr float KOOPA_SHELL_SPEED         = 180.0f;  // SMB1 滑行龜殼速度（世界像素/秒）
constexpr float KOOPA_REVIVE_DELAY        =   5.0f;  // 靜止殼開始復活前的總等待時間
constexpr float KOOPA_REVIVE_WARNING      =   1.5f;  // 復活前切換伸腳警告圖的時間
constexpr float KOOPA_DEFEAT_X_SPEED      =  60.0f;
constexpr float KOOPA_DEFEAT_Y_SPEED      = 280.0f;
constexpr float PARATROOPA_JUMP_SPEED     = 250.0f;

/*
 * MAX_FRAME_DT：單幀可接受的最大秒數
 *
 * 啟動載資源或視窗暫停後，第一幀 dt 可能偏大。
 * 若直接拿來更新物理，角色可能一幀穿過地板。
 */
constexpr float MAX_FRAME_DT = 1.0f / 30.0f;

// ─── 視窗 ─────────────────────────────────────────────────────
// 視窗尺寸定義在 PTSD/include/config.hpp
// WINDOW_WIDTH = 960, WINDOW_HEIGHT = 720

// ─── 渲染（Z-Index / 圖層） ──────────────────────────────────
/*
 * 定義各種實體在畫面上的前後遮擋關係 (Z-Index)
 * 數字越小，畫在越後面（底層）。
 */
constexpr float Z_INDEX_BACKGROUND = -1.0f;

#endif
