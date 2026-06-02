#ifndef BRICK_DEBRIS_HPP
#define BRICK_DEBRIS_HPP

#include <memory>
#include <string>

#include "Util/GameObject.hpp"

class Camera;

class BrickDebris : public Util::GameObject {
public:
    BrickDebris(glm::vec2 position, glm::vec2 velocity, const std::string& theme);

    void Update(float deltaTime);
    void Draw(const Camera& camera);

    bool IsDead() const { return m_Lifetime <= 0.0f; }

private:
    glm::vec2 m_Position = {0.0f, 0.0f};
    glm::vec2 m_Size = {8.0f, 8.0f};
    glm::vec2 m_Velocity = {0.0f, 0.0f};
    float m_Lifetime = 0.9f;
    float m_Gravity = 900.0f;
};

#endif
