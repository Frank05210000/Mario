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
                        float moveSpeed,
                        const std::string& moveMode = "oscillate",
                        const std::string& startDirection = "",
                        float startOffset = 0.0f);

    Type GetType() const override { return Type::MovingPlatform; }
    bool IsSolid() const override { return true; }
    bool IsOneWay() const override { return true; }  // 原版升降梯可從下方穿過
    void Update(float deltaTime) override;
    glm::vec2 GetFrameDelta() const { return m_FrameDelta; }

private:
    glm::vec2 m_StartPosition = {0.0f, 0.0f};
    std::string m_MoveAxis = "horizontal";
    std::string m_MoveMode = "oscillate";
    std::string m_StartDirection;
    float m_MoveDistance = 0.0f;
    float m_MoveSpeed = 0.0f;
    int m_StartSign = 1;
    int m_Direction = 1;
    glm::vec2 m_FrameDelta = {0.0f, 0.0f};
};

#endif
