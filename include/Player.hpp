#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <memory>

#include "Entity.hpp"
#include "Util/GameObject.hpp"

class Player {
public:
    enum class Form {
        SMALL,
        SUPER,
        FIRE,
    };

    Player();

    void update();
    void render(float cameraX);
    void ClampToCameraBounds(float cameraX);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetSize() const { return m_Size; }
    Form GetForm() const { return m_Form; }
    std::shared_ptr<Util::GameObject> GetGameObject() const { return m_Object; }

private:
    void ApplyJump();
    void ApplyGravity();
    void ResolveGroundCollision();
    void ClampToWorldBounds();
    glm::vec2 ToScenePosition(float cameraX) const;
    void EnsureGameObject();

    Vec2 m_Position {};
    Vec2 m_Size {};
    float m_Speed = 5.0F;
    float m_VelocityY = 0.0F;
    float m_Gravity = 0.8F;
    float m_JumpStrength = 16.0F;
    float m_GroundY = 417.92F;
    bool m_OnGround = true;
    Form m_Form = Form::SMALL;
    std::shared_ptr<Util::GameObject> m_Object;
};

#endif
