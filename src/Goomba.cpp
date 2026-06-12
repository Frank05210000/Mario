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

    SetDrawable(m_WalkAnim);
}

void Goomba::Update(float deltaTime) {
    if (!m_IsAlive) return;

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

void Goomba::Stomp() {
    if (m_IsSquashed) return; // 避免重複觸發

    // 進入壓扁狀態：顯示壓扁圖、停止移動、啟動計時
    m_IsSquashed  = true;
    m_SquashTimer = SQUASH_DURATION;
    m_Velocity.x  = 0.0f;
    m_Velocity.y  = 0.0f;
    SetDrawable(m_StompImage);
}
