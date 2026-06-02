#ifndef MOVING_PLATFORM_BLOCK_HPP
#define MOVING_PLATFORM_BLOCK_HPP

#include <string>

#include "Block.hpp"

class MovingPlatformBlock : public Block {
public:
    MovingPlatformBlock(glm::vec2 position,
                        glm::vec2 size,
                        const std::string& moveAxis,
                        float moveDistance,
                        float moveSpeed);

    Type GetType() const override { return Type::MovingPlatform; }
    bool IsSolid() const override { return true; }
    void Update(float deltaTime) override;

private:
    glm::vec2 m_StartPosition = {0.0f, 0.0f};
    std::string m_MoveAxis = "horizontal";
    float m_MoveDistance = 0.0f;
    float m_MoveSpeed = 0.0f;
    int m_Direction = 1;
};

#endif
