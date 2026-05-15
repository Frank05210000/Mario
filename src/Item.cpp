#include "Item.hpp"

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
}

void Item::ApplyGravity(float deltaTime) {
    if (m_State != ItemState::Active) return;

    m_Velocity.y += m_Gravity * deltaTime;
    m_Position.y += m_Velocity.y * deltaTime;
}
