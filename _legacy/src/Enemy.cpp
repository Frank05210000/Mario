#include "Enemy.hpp"

Enemy::Enemy(Type type, Vec2 position) : Entity(position), m_Type(type) {
    m_Velocity.x = -kWalkSpeed;
}

void Enemy::Update(float deltaTime) {
    m_Velocity.y += kGravity * deltaTime;
    Entity::Update(deltaTime);
}

void Enemy::ReverseDirection() {
    m_Velocity.x = -m_Velocity.x;
}

void Enemy::Stomp() {
    m_Active = false;
}
