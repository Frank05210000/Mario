#include "Character.hpp"

void Character::ApplyGravity(float deltaTime) {
    // 如果角色已經死了，不需要套用重力
    if (!m_IsAlive) return;

    // 速度 y 方向持續增加（往下加速）
    // deltaTime 是這幀花了多少秒，讓物理效果不受幀率影響
    m_Velocity.y += m_Gravity * deltaTime;

    // 把速度套用到位置上
    m_Position.y += m_Velocity.y * deltaTime;
}
