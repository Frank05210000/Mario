#include "Item.hpp"

#include "Core/Context.hpp"

void Item::Draw(const Camera& camera) {
    if (m_State == ItemState::Collected) {
        SetVisible(false);
        return;
    }

    SetVisible(true);

    glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    // 自方塊冒出期間，沿「方塊頂線」(m_SpawnBaseY) 裁切，只露出頂線以上部分。
    if (m_Clip) {
        if (m_State == ItemState::Spawning) {
            const auto  ctx   = Core::Context::GetInstance();
            const float halfH = static_cast<float>(ctx->GetWindowHeight()) * 0.5f;
            // framebuffer 像素 Y（原點左下）= 螢幕中心座標.y + 半個視窗高
            const float topPx =
                camera.WorldToScreen({m_Position.x, m_SpawnBaseY}).y + halfH;
            m_Clip->SetClipKeepAbovePx(topPx);
            m_Clip->SetEnabled(true);
        } else {
            m_Clip->SetEnabled(false);
        }
    }
}

void Item::ApplyGravity(float deltaTime) {
    if (m_State != ItemState::Active) return;

    m_Velocity.y += m_Gravity * deltaTime;
    m_Position.y += m_Velocity.y * deltaTime;
}
