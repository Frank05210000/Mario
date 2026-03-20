#include "Camera.hpp"

#include <algorithm>

#include "config.hpp"

void Camera::Update(float playerX, float worldWidth) {
    const float viewportWidth = static_cast<float>(RENDER_WIDTH);
    const float maxCameraX = std::max(0.0F, worldWidth - viewportWidth);
    const float playerScreenX = playerX - m_X;

    if (m_TriggerX == 0.0F) {
        m_TriggerX = viewportWidth * 0.5F;
    }

    if (playerScreenX <= m_TriggerX) {
        return;
    }

    const float desiredCameraX = playerX - m_TriggerX;
    const float clampedCameraX = std::clamp(desiredCameraX, 0.0F, maxCameraX);
    m_X = std::max(m_X, clampedCameraX);
}
