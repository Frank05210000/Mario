#include "Fireball.hpp"
#include "AssetPath.hpp"
#include "GameConstants.hpp"

Fireball::Fireball(glm::vec2 pos, bool movingLeft) {
    m_Position = pos;
    m_Size = {16.0f, 16.0f}; // 假設火球大小大約為 16x16
    
    std::vector<std::string> animPaths = {
        MakeAssetPath("item/effect/fireball/small/fireball-1.png"),
        MakeAssetPath("item/effect/fireball/small/fireball-2.png"),
        MakeAssetPath("item/effect/fireball/small/fireball-3.png"),
        MakeAssetPath("item/effect/fireball/small/fireball-4.png")
    };
    
    std::vector<std::string> burstPaths = {
        MakeAssetPath("item/effect/fireball/burst/burst-1.png"),
        MakeAssetPath("item/effect/fireball/burst/burst-2.png"),
        MakeAssetPath("item/effect/fireball/burst/burst-3.png")
    };
    
    m_Animation = std::make_shared<Util::Animation>(animPaths, true, 50, true);
    m_BurstAnimation = std::make_shared<Util::Animation>(burstPaths, true, 50, false);
    
    SetDrawable(m_Animation);
    SetZIndex(8.0f); // 畫在前面

    const float speedX = 320.0f;
    m_Gravity = 1400.0f;
    m_BounceForce = 220.0f;
    m_Velocity.x = movingLeft ? -speedX : speedX;
    m_Velocity.y = -60.0f;
}

void Fireball::Update(float deltaTime) {
    if (m_IsDead) return;
    
    if (m_IsExploded) {
        m_ExplodeTimer -= deltaTime;
        if (m_ExplodeTimer <= 0.0f) {
            m_IsDead = true;
            SetVisible(false);
        }
        return;
    }

    m_Velocity.y += m_Gravity * deltaTime;
    m_Position.x += m_Velocity.x * deltaTime;
    m_Position.y += m_Velocity.y * deltaTime;
}

void Fireball::Draw(const Camera& camera) {
    if (m_IsDead) return;
    
    glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f
    };
    
    m_Transform.translation = camera.WorldToScreen(centerPos);
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};
}

void Fireball::Bounce() {
    m_Velocity.y = -m_BounceForce;
}

void Fireball::Explode() {
    if (m_IsExploded) return;

    m_IsExploded = true;
    m_Velocity = {0.0f, 0.0f}; // 停止移動
    
    SetDrawable(m_BurstAnimation); // 切換到爆炸動畫
    m_BurstAnimation->Play(); // 確保動畫從頭開始播
    
    m_ExplodeTimer = 0.15f; // 3個 frame，每個 50ms = 0.15 秒
}
