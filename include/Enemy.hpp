#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Character.hpp"
#include "GameConstants.hpp"

/*
 * Enemy：所有敵人的抽象基底類別
 *
 * 繼承 Character，加入敵人共用行為：自動走路、掉頭。
 * Stomp() 為純虛擬，各子類別（Goomba、Koopa）自行決定被踩後的行為。
 *
 * OOP 關係：
 *   Character ← Enemy ← Goomba
 *                      ← Koopa
 */
class Enemy : public Character {
public:
    Enemy();
    virtual ~Enemy() = default;

    // ─── 共用邏輯（Goomba / Koopa 都一樣）────────────────────────

    /* 每幀更新
     * 套用重力 → 水平移動。
     * 子類別可再 override 加入自己的行為。
     */
    void Update(float deltaTime) override;

    /* 每幀同步畫面位置
     * 把世界座標換算成螢幕座標 (套用 GAME_SCALE)
     */
    void Draw(const Camera& camera) override;

    /* 掉頭
     * 碰到牆壁或地圖邊界時呼叫，反轉水平速度。
     * Goomba 和 Koopa 行為相同，放在 Enemy。
     */
    void ReverseDirection();

    // ─── 純虛擬（子類別必須各自實作）────────────────────────────

    /* 被踩
     * Goomba：直接死亡
     * Koopa ：縮進殼裡
     */
    virtual void Stomp() = 0;

protected:
    float m_WalkSpeed = 40.0f;  // 行走速度（像素/秒），子類別可覆蓋（原版 NES 栗寶寶 ≈ 0.75 tiles/s × 16 × 3 ≈ 36，取 40）

private:

};

#endif
