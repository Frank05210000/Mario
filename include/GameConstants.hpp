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
 * GAME_SCALE：全域畫面縮放比例
 *
 * NES 原始高度是 240px。
 * PTSD 視窗高度預設為 720px。
 * 統一將所有渲染物件放大 3.0 倍 (240 * 3 = 720)，確保畫面完美填滿。
 */
constexpr float GAME_SCALE = 3.0f;

// ─── 玩家物理：垂直（世界像素/秒；未乘 GAME_SCALE）────────────────
constexpr float PLAYER_JUMP_VELOCITY      = 250.0f;  // 跳躍初速；全按住 ≈ 4.4 tiles
constexpr float PLAYER_JUMP_RUN_BONUS     =  30.0f;  // 跑跳加成 → 280 ≈ 5.4 tiles
constexpr float PLAYER_RUN_JUMP_THRESHOLD =  90.0f;  // |vx| 超過走路極速才吃 bonus
constexpr float GRAVITY_RISE              = 450.0f;  // 上升中且按住跳：弱重力
constexpr float GRAVITY_FALL              = 1250.0f; // 放開跳 或 vy>=0：強重力（2.78x）
constexpr float MAX_FALL_SPEED            = 400.0f;  // 終端速度：400/30 ≈ 13.3px < 16

// ─── 玩家物理：水平 ───────────────────────────────────────────────────────
constexpr float PLAYER_MAX_WALK_SPEED     =  90.0f;  // 走路最高速（NES ≈ 93.75）
constexpr float PLAYER_MAX_RUN_SPEED      = 160.0f;  // 奔跑最高速（NES ≈ 153.75；現值 175 偏滑）
constexpr float PLAYER_ACCELERATION       = 200.0f;  // 水平加速度（沿用現值）
constexpr float PLAYER_RUN_ACCEL_MULT     =   1.5f;  // 地面奔跑加速度倍率（沿用現有 *1.5 邏輯）
constexpr float PLAYER_SKID_DECEL         = 400.0f;  // 轉向煞車（打滑，沿用現值）
constexpr float PLAYER_FRICTION           = 200.0f;  // 放開方向鍵摩擦（沿用現值）

// ─── 其他 ─────────────────────────────────────────────────────────────────
constexpr float PLAYER_DEATH_BOUNCE       = 500.0f;  // 死亡上彈初速（取代 Player.cpp 硬編碼 -500）
constexpr float ENEMY_GRAVITY             = 700.0f;  // 敵人/死亡動畫重力（沿用 Character 現值）

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
