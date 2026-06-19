#ifndef FLAG_BLOCK_HPP
#define FLAG_BLOCK_HPP

#include "Block.hpp"
#include "GameConstants.hpp"

/*
 * FlagBlock：終點旗杆
 *
 * JSON 只記錄旗杆底部的座標，
 * 程式自動根據 FLAG_POLE_TILES 往上推算偵測範圍。
 *
 * 計分邏輯：
 *   玩家碰到旗杆時，依接觸高度回傳不同分數（越高越多）。
 *
 * 旗球下降動畫
 *   觸碰旗杆後呼叫 StartDescent()，球從杆頂滑向杆底。
 *   TODO: 目前 Resources/Asset/item/flag/ 只有 ball.png，沒有旗幟 flag sprite。
 *         若後續取得 flag.png，可在此加入子 GameObject 同步下移。
 */
class FlagBlock : public Block {
public:
    explicit FlagBlock(glm::vec2 bottomPosition,
                       float clearWalkTiles = DEFAULT_LEVEL_CLEAR_WALK_TILES,
                       float castleFlagBaseTiles = DEFAULT_CASTLE_FLAG_BASE_TILES);

    Type GetType() const override { return Type::Flag; }
    bool IsSolid() const override { return false; }
    void Update(float dt) override;
    void Draw(const Camera& camera) override;

    /*
     * 根據玩家碰到旗杆的 Y 世界座標，計算對應得分。
     *
     * @param playerY  玩家腳底的世界 Y 座標
     * @return         對應分數（5000 / 2000 / 800 / 400 / 100）
     *
     * ratio = (旗杆底部 - 玩家Y) / 旗杆總高
     * ratio 越大代表碰得越高，分數越高。
     */
    int GetContactScore(float playerY) const;
    float GetClearWalkTiles() const { return m_ClearWalkTiles; }
    float GetCastleFlagBaseTiles() const { return m_CastleFlagBaseTiles; }

    /*
     * 開始旗球下降動畫（由 GameManager::CheckFlagCollision 觸發）
     */
    void StartDescent();

    bool IsDescending() const { return m_IsDescending; }

private:
    // ─── 旗球下降動畫狀態 ──────────────────────────────────────────
    bool  m_IsDescending = false;  // 是否正在下降中
    float m_BallOffsetY  = 0.0f;   // 旗子相對杆頂的 Y 偏移（世界像素，往下增加）
    float m_BallTargetY  = 0.0f;   // 下降目標 Y（杆底）
    float m_ClearWalkTiles = DEFAULT_LEVEL_CLEAR_WALK_TILES;
    float m_CastleFlagBaseTiles = DEFAULT_CASTLE_FLAG_BASE_TILES;
    // 旗子與馬力歐用同一下滑速度（POLE_SLIDE_SPEED），確保兩者同步到底
    static constexpr float DESCENT_SPEED = POLE_SLIDE_SPEED; // 下降速度（世界像素/秒）
};

#endif
