#include "Enemy.hpp"
#include "Camera.hpp"
#include "Core/Context.hpp"

Enemy::Enemy() {
    // 一開始往左走（速度 x 為負）
    m_Velocity.x = -m_WalkSpeed;
    SetZIndex(5.0f);
}

void Enemy::Update(float deltaTime) {
    if (!m_IsAlive) return;

    // 1. 套用重力
    ApplyGravity(deltaTime);

    // 2. 水平移動
    m_Position.x += m_Velocity.x * deltaTime;
}

void Enemy::Draw(const Camera& camera) {
    if (!m_IsAlive) {
        SetVisible(false);
        return;
    }
    SetVisible(true);
    glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);
}

void Enemy::ReverseDirection() {
    m_Velocity.x = -m_Velocity.x;
}

