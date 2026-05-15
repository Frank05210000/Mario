#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "Entity.hpp"

class Enemy : public Entity {
public:
    enum class Type {
        GOOMBA,
        KOOPA,
    };

    Enemy(Type type, Vec2 position);

    void Update(float deltaTime) override;
    void ReverseDirection();
    void Stomp();

    Type GetType() const { return m_Type; }

private:
    static constexpr float kWalkSpeed = 70.0F;
    static constexpr float kGravity = 900.0F;

    Type m_Type;
};

#endif
