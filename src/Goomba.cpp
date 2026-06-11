#include "Goomba.hpp"

#include <vector>

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

Goomba::Goomba(float startX, float startY, const std::string& theme) {
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE};      // 世界尺寸：16x16
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    // 2 幀行走動畫（walk-1 + walk-2，每幀 200ms，循環）
    // 依主題載入對應路徑
    m_WalkAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath("enemy/Goomba/" + theme + "/normal/walk/walk-1.png"),
            MakeAssetPath("enemy/Goomba/" + theme + "/normal/walk/walk-2.png"),
        },
        true,   // hasMultipleFrames
        200,    // frameTimeMs
        true);  // isLooping

    // 壓扁圖片
    m_StompImage = std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Goomba/" + theme + "/normal/stomp/stomp.png"));

    SetDrawable(m_WalkAnim);
}

void Goomba::Update(float deltaTime) {
    if (!m_IsAlive) return;

    if (m_IsSquashed) {
        // 壓扁倒數中：停在原位，等待動畫結束後死亡
        m_SquashTimer -= deltaTime;
        if (m_SquashTimer <= 0.0f) {
            SetAlive(false);
        }
        return;
    }

    // 正常行走：使用父類邏輯（重力 + 水平移動）
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
