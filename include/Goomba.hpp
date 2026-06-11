#ifndef GOOMBA_HPP
#define GOOMBA_HPP

#include "Enemy.hpp"
#include "ThemeAssets.hpp"

/*
 * Goomba：栗寶寶
 *
 * 繼承 Enemy，實作 Stomp()。
 * 被踩到後直接死亡（SetAlive(false)）。
 *
 * 行走邏輯沿用 Enemy::Update()，不需要覆寫。
 */
class Goomba : public Enemy {
public:
    /* 建構子
     * startX / startY：出生的世界座標位置。
     */
    Goomba(float startX, float startY, const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    /* 被踩：直接死亡
     * 栗寶寶沒有殼，踩一下就消失。
     */
    void Stomp() override;
};

#endif
