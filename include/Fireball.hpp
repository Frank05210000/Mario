#ifndef FIREBALL_HPP
#define FIREBALL_HPP

#include "Util/GameObject.hpp"
#include "Util/Animation.hpp"
#include "Camera.hpp"
#include <memory>

class Fireball : public Util::GameObject {
public:
    Fireball(glm::vec2 pos, bool movingLeft);

    void Update(float deltaTime);
    void Draw(const Camera& camera);

    void Bounce();
    void Explode();

    glm::vec2 GetPosition() const { return m_Position; }
    void SetPosition(glm::vec2 pos) { m_Position = pos; }
    glm::vec2 GetSize() const { return m_Size; }
    
    glm::vec2 GetVelocity() const { return m_Velocity; }
    void SetVelocity(glm::vec2 vel) { m_Velocity = vel; }

    bool IsExploded() const { return m_IsExploded; }
    bool IsDead() const { return m_IsDead; }

private:
    glm::vec2 m_Position = {0.0f, 0.0f};
    glm::vec2 m_Velocity = {0.0f, 0.0f};
    glm::vec2 m_Size = {8.0f, 8.0f};

    float m_Gravity = 1500.0f;
    float m_BounceForce = 140.0f;
    
    bool m_IsExploded = false;
    bool m_IsDead = false;
    float m_ExplodeTimer = 0.0f;
    
    std::shared_ptr<Util::Animation> m_Animation;
    std::shared_ptr<Util::Animation> m_BurstAnimation;
};

#endif
