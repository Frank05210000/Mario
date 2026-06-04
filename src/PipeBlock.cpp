#include "PipeBlock.hpp"

#include "GameConstants.hpp"
#include "Player.hpp"

#include <cmath>

namespace {
constexpr float PIPE_ENTRY_TOLERANCE = TILE_SIZE * 0.5f;
}

PipeBlock::PipeBlock(glm::vec2 position,
                     glm::vec2 size,
                     const std::string& opening)
    : Block(position, size),
      m_Opening(opening) {}

bool PipeBlock::CanEnter(const Player&,
                         bool,
                         bool,
                         bool,
                         bool) const {
    return false;
}

std::string PipeBlock::GetDestinationLevel() const {
    return "";
}

std::optional<glm::vec2> PipeBlock::GetDestinationSpawn() const {
    return std::nullopt;
}

EnterablePipeBlock::EnterablePipeBlock(glm::vec2 position,
                                       glm::vec2 size,
                                       const std::string& opening,
                                       const std::string& targetLevel,
                                       const std::string& exitToLevel,
                                       glm::vec2 targetSpawn,
                                       bool hasTargetSpawn)
    : PipeBlock(position, size, opening),
      m_TargetLevel(targetLevel),
      m_ExitToLevel(exitToLevel),
      m_TargetSpawn(targetSpawn),
      m_HasTargetSpawn(hasTargetSpawn) {}

bool EnterablePipeBlock::CanEnter(const Player& player,
                                  bool pressingUp,
                                  bool pressingDown,
                                  bool pressingLeft,
                                  bool pressingRight) const {
    const glm::vec2 playerPos = player.GetPosition();
    const glm::vec2 playerSize = player.GetSize();
    const float playerCenterX = playerPos.x + playerSize.x * 0.5f;
    const float playerCenterY = playerPos.y + playerSize.y * 0.5f;
    const float playerBottom = playerPos.y + playerSize.y;

    if (m_Opening == "up") {
        const bool horizontallyAligned =
            playerCenterX >= m_Position.x &&
            playerCenterX <= m_Position.x + m_Size.x;
        const bool standingOnMouth =
            std::abs(playerBottom - m_Position.y) <= PIPE_ENTRY_TOLERANCE;
        return pressingDown && horizontallyAligned && standingOnMouth;
    }

    if (m_Opening == "down") {
        const bool horizontallyAligned =
            playerCenterX >= m_Position.x &&
            playerCenterX <= m_Position.x + m_Size.x;
        const bool touchingMouth =
            std::abs(playerPos.y - (m_Position.y + m_Size.y)) <= PIPE_ENTRY_TOLERANCE;
        return pressingUp && horizontallyAligned && touchingMouth;
    }

    if (m_Opening == "left") {
        const bool verticallyAligned =
            playerCenterY >= m_Position.y &&
            playerCenterY <= m_Position.y + m_Size.y;
        const bool touchingMouth =
            std::abs((playerPos.x + playerSize.x) - m_Position.x) <= PIPE_ENTRY_TOLERANCE;
        return pressingRight && verticallyAligned && touchingMouth;
    }

    if (m_Opening == "right") {
        const bool verticallyAligned =
            playerCenterY >= m_Position.y &&
            playerCenterY <= m_Position.y + m_Size.y;
        const bool touchingMouth =
            std::abs(playerPos.x - (m_Position.x + m_Size.x)) <= PIPE_ENTRY_TOLERANCE;
        return pressingLeft && verticallyAligned && touchingMouth;
    }

    return false;
}

std::string EnterablePipeBlock::GetDestinationLevel() const {
    if (!m_TargetLevel.empty()) return m_TargetLevel;
    return m_ExitToLevel;
}

std::optional<glm::vec2> EnterablePipeBlock::GetDestinationSpawn() const {
    if (!m_HasTargetSpawn) return std::nullopt;
    return m_TargetSpawn;
}

PipeCollisionBlock::PipeCollisionBlock(glm::vec2 position, glm::vec2 size)
    : PipeBlock(position, size, "up") {}
