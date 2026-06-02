#include "MovingPlatformBlock.hpp"

#include <algorithm>

MovingPlatformBlock::MovingPlatformBlock(glm::vec2 position,
                                         glm::vec2 size,
                                         const std::string& moveAxis,
                                         float moveDistance,
                                         float moveSpeed)
    : Block(position, size),
      m_StartPosition(position),
      m_MoveAxis(moveAxis),
      m_MoveDistance(std::max(0.0f, moveDistance)),
      m_MoveSpeed(std::max(0.0f, moveSpeed)) {
    SetSprite("block/platform/moving_platform.png", 2.0f);
}

void MovingPlatformBlock::Update(float deltaTime) {
    if (m_MoveDistance <= 0.0f || m_MoveSpeed <= 0.0f) return;

    glm::vec2 axis = {1.0f, 0.0f};
    if (m_MoveAxis == "vertical") {
        axis = {0.0f, 1.0f};
    }

    m_Position += axis * (m_MoveSpeed * static_cast<float>(m_Direction) * deltaTime);

    const float offset =
        (m_MoveAxis == "vertical") ? (m_Position.y - m_StartPosition.y)
                                   : (m_Position.x - m_StartPosition.x);
    if (offset > m_MoveDistance) {
        m_Position = m_StartPosition + axis * m_MoveDistance;
        m_Direction = -1;
    } else if (offset < 0.0f) {
        m_Position = m_StartPosition;
        m_Direction = 1;
    }
}
