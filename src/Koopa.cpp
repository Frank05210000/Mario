#include "Koopa.hpp"

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

Koopa::Koopa(float startX, float startY) {
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE * 1.5f};  // 世界尺寸：16x24
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    SetDrawable(std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Koopa/ground/normal/walk/walk-1.png")));
}

void Koopa::Update(float deltaTime) {
    if (m_InShell && !m_IsSliding) {
        // 縮殼停止狀態：只套用重力，不走路
        ApplyGravity(deltaTime);
        return;
    }

    if (m_IsSliding) {
        // 殼滑行狀態：套用重力 + 水平快速移動（由 Enemy 的碰撞系統控制掴頭）
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        return;
    }

    // 正常狀態：水平移動 + 重力（決於 Enemy::Update）
    Enemy::Update(deltaTime);
}

void Koopa::Stomp() {
    // 龜：縮進殼裡，停止走路
    m_InShell = true;
    m_IsSliding = false;
    m_Velocity.x = 0.0f;

    // 換成殼的圖片
    SetDrawable(std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Koopa/ground/normal/shell/shell.png")));
        
    // 龜殼狀態身高變為 1 格高，並下降 1 格避免浮空
    if (m_Size.y > TILE_SIZE) {
        m_Position.y += (m_Size.y - TILE_SIZE);
        m_Size.y = TILE_SIZE;
    }
}

void Koopa::Kick(bool kickLeft) {
    // 縮殼被踢出：朝踢出方向橫向高速滑行
    // 原版 NES 殼速度 ≈ 10 tiles/s × 16 × 3 = 480，取略保守 380
    m_IsSliding = true;
    const float SHELL_SPEED = 380.0f;
    m_Velocity.x = kickLeft ? -SHELL_SPEED : SHELL_SPEED;
}
