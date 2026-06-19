#include "MovingPlatformBlock.hpp"

#include <algorithm>
#include <cmath>

namespace {
int DirectionSignForAxis(const std::string& moveAxis, const std::string& startDirection) {
    if (moveAxis == "vertical") {
        return startDirection == "up" ? -1 : 1;
    }
    return startDirection == "left" ? -1 : 1;
}
}

MovingPlatformBlock::MovingPlatformBlock(glm::vec2 position,
                                         glm::vec2 size,
                                         const std::string& moveAxis,
                                         float moveDistance,
                                         float moveSpeed,
                                         const std::string& moveMode,
                                         const std::string& startDirection,
                                         float startOffset)
    : Block(position, size),
      m_StartPosition(position),
      m_MoveAxis(moveAxis),
      m_MoveMode(moveMode),
      m_StartDirection(startDirection.empty() ? (moveAxis == "vertical" ? "down" : "right") : startDirection),
      m_MoveDistance(std::max(0.0f, moveDistance)),
      m_MoveSpeed(std::max(0.0f, moveSpeed)),
      m_StartSign(DirectionSignForAxis(m_MoveAxis, m_StartDirection)),
      m_Direction(m_StartSign) {
    SetSprite("block/platform/moving_platform.png", 2.0f);

    // 初始相位偏移：沿行程方向先位移 startOffset 像素（夾在行程範圍內）。
    // 讓同一組平台可以錯開相位，不會整排同進同退（JSON: startOffsetTiles）。
    const float clampedOffset = std::clamp(startOffset, 0.0f, m_MoveDistance);
    if (clampedOffset > 0.0f) {
        glm::vec2 axis = (m_MoveAxis == "vertical") ? glm::vec2{0.0f, 1.0f}
                                                    : glm::vec2{1.0f, 0.0f};
        m_Position += axis * (clampedOffset * static_cast<float>(m_StartSign));
    }
}

void MovingPlatformBlock::Update(float deltaTime) {
    const glm::vec2 previousPosition = m_Position;
    m_FrameDelta = {0.0f, 0.0f};
    if (m_MoveDistance <= 0.0f || m_MoveSpeed <= 0.0f) return;

    glm::vec2 axis = {1.0f, 0.0f};
    if (m_MoveAxis == "vertical") {
        axis = {0.0f, 1.0f};
    }

    m_Position += axis * (m_MoveSpeed * static_cast<float>(m_Direction) * deltaTime);

    const float rawOffset =
        (m_MoveAxis == "vertical") ? (m_Position.y - m_StartPosition.y)
                                   : (m_Position.x - m_StartPosition.x);
    const float offset = rawOffset * static_cast<float>(m_StartSign);

    if (m_MoveMode == "verticalWrap" && m_MoveAxis == "vertical") {
        // 循環模式：超出行程後回到另一端，保留 overshoot 讓速度不在端點頓一下。
        // 注意：瞬移量不得計入 m_FrameDelta，否則站在平台上的玩家會被一起傳送；
        // 乘客應該留在原地（平台從腳下消失，下一幀自然下落）。
        m_FrameDelta = m_Position - previousPosition; // 只含平滑移動量
        if (offset > m_MoveDistance || offset < 0.0f) {
            float wrappedOffset = std::fmod(offset, m_MoveDistance);
            if (wrappedOffset < 0.0f) {
                wrappedOffset += m_MoveDistance;
            }
            m_Position = m_StartPosition + axis * (wrappedOffset * static_cast<float>(m_StartSign));
        }
        return;
    }

    if (offset > m_MoveDistance) {
        m_Position = m_StartPosition + axis * (m_MoveDistance * static_cast<float>(m_StartSign));
        m_Direction = -m_StartSign;
    } else if (offset < 0.0f) {
        m_Position = m_StartPosition;
        m_Direction = m_StartSign;
    }

    m_FrameDelta = m_Position - previousPosition;
}
