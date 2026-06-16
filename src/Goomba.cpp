#include "Goomba.hpp"

#include <vector>

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

Goomba::Goomba(float startX, float startY, const ThemeAssets& assets) {
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    m_WalkAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            assets.Sprite("enemy/Goomba/{theme}/normal/walk/walk-1.png"),
            assets.Sprite("enemy/Goomba/{theme}/normal/walk/walk-2.png"),
        },
        true, 200, true);

    m_StompImage = std::make_shared<Util::Image>(
        assets.Sprite("enemy/Goomba/{theme}/normal/stomp/stomp.png"));

    // 翻倒死亡圖：沿用正常行走第一幀，於 Die() 時上下翻轉繪製
    m_DeadImage = std::make_shared<Util::Image>(
        assets.Sprite("enemy/Goomba/{theme}/normal/walk/walk-1.png"));

    SetDrawable(m_WalkAnim);
}

void Goomba::Update(float deltaTime) {
    if (!m_IsAlive) return;

    if (m_IsDead) {
        // 翻倒死亡：套用重力 + 水平拋飛，墜出畫面後由 GameManager 的 killZ 清除
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        return;
    }

    if (m_IsSquashed) {
        m_SquashTimer -= deltaTime;
        if (m_SquashTimer <= 0.0f) {
            SetAlive(false);
        }
        return;
    }

    SetDrawable(m_WalkAnim);
    Enemy::Update(deltaTime);
}

Enemy::StompOutcome Goomba::Stomp() {
    if (m_IsSquashed) return StompOutcome::NoEffect;

    // 進入壓扁狀態：顯示壓扁圖、停止移動、啟動計時
    m_IsSquashed  = true;
    m_SquashTimer = SQUASH_DURATION;
    m_Velocity.x  = 0.0f;
    m_Velocity.y  = 0.0f;
    SetDrawable(m_StompImage);
    return StompOutcome::Defeated;
}

void Goomba::Die(bool flipLeft) {
    if (m_IsDead || m_IsSquashed || !m_IsAlive) return;

    // 翻倒死亡：上下翻轉 + 向上彈起再墜落（與 Koopa::Die 相同的翻倒軌跡）
    m_IsDead     = true;
    m_IsGrounded = false;
    m_Velocity.y = -KOOPA_DEFEAT_Y_SPEED;
    m_Velocity.x = flipLeft ? -KOOPA_DEFEAT_X_SPEED : KOOPA_DEFEAT_X_SPEED;
    SetDrawable(m_DeadImage);
    m_Transform.scale = {GAME_SCALE, -GAME_SCALE};  // y 軸負縮放 → 上下翻轉
    SetVisible(true);
}
