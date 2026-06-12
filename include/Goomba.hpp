#ifndef GOOMBA_HPP
#define GOOMBA_HPP

#include <memory>

#include "Enemy.hpp"
#include "ThemeAssets.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

/*
 * Goomba：栗寶寶
 *
 * 繼承 Enemy，實作 Stomp()。
 * 被踩到後進入 ~0.4s 的壓扁動畫，再設置 SetAlive(false)。
 * 壓扁狀態中不具傷害性。
 *
 * 行走邏輯：改用 2 幀 Util::Animation 動畫 (walk-1, walk-2)。
 */
class Goomba : public Enemy {
public:
    /* 建構子
     * startX / startY：出生的世界座標位置。
     * assets：主題資產解析器，用來載入對應主題的精靈。
     */
    Goomba(float startX, float startY, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    /* 每幀更新（覆寫 Enemy::Update）
     * 壓扁倒數計時 → 消滅；其他時候套用父類邏輯。
     */
    void Update(float deltaTime) override;

    /* 被踩：進入壓扁狀態
     * 顯示壓扁圖片，啟動倒數計時後才真正死亡。
     */
    void Stomp() override;

    /* 壓扁中不具傷害性 */
    bool IsSquashed() const { return m_IsSquashed; }

private:
    bool   m_IsSquashed   = false;   // 是否正在壓扁狀態
    float  m_SquashTimer  = 0.0f;    // 壓扁倒數（秒）
    static constexpr float SQUASH_DURATION = 0.4f;

    std::shared_ptr<Util::Animation> m_WalkAnim;   // 2 幀行走動畫
    std::shared_ptr<Util::Image>     m_StompImage; // 壓扁圖片
};

#endif
