#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <memory>

#include "Entity.hpp"
#include "Util/GameObject.hpp"

class Map;

class Player {
public:
    enum class Form {
        SMALL,
        SUPER,
        FIRE,
    };

    Player();

    void update(const Map& map);
    void render(float cameraX);
    void ClampToCameraBounds(float cameraX);
    void SetSpawnPosition(Vec2 position);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetSize() const { return m_Size; }
    Form GetForm() const { return m_Form; }
    std::shared_ptr<Util::GameObject> GetGameObject() const { return m_Object; }

private:
    void ApplyJump();
    void ApplyGravity();
    void ResolveHorizontalCollisions(const Map& map, float previousX);
    void ResolveVerticalCollisions(const Map& map, float previousY);
    bool HasGroundSupport(const Map& map) const;
    void ClampToWorldBounds(float worldWidth);
    glm::vec2 ToScenePosition(float cameraX) const;
    void EnsureGameObject();

    Vec2 m_Position {};
    Vec2 m_Size {};
    float m_Speed = 5.0F;
    float m_VelocityY = 0.0F;
    float m_Gravity = 0.8F;
    float m_JumpStrength = 16.0F;
    bool m_OnGround = true;
    Form m_Form = Form::SMALL;
    std::shared_ptr<Util::GameObject> m_Object;
};

#endif
