#ifndef ENTITY_HPP
#define ENTITY_HPP

struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;
};

class Entity {
public:
    Entity() = default;
    explicit Entity(Vec2 position) : m_Position(position) {}
    virtual ~Entity() = default;

    virtual void Update(float deltaTime);

    Vec2 GetPosition() const { return m_Position; }
    Vec2 GetVelocity() const { return m_Velocity; }
    bool IsActive() const { return m_Active; }

    void SetPosition(Vec2 position) { m_Position = position; }
    void SetVelocity(Vec2 velocity) { m_Velocity = velocity; }
    void SetActive(bool active) { m_Active = active; }

protected:
    Vec2 m_Position {};
    Vec2 m_Velocity {};
    bool m_Active = true;
};

#endif
