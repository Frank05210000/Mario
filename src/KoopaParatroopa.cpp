#include "KoopaParatroopa.hpp"

#include <cmath>

#include "GameConstants.hpp"
#include "Util/Logger.hpp"

KoopaParatroopa::KoopaParatroopa(float startX,
                                 float startY,
                                 Variant variant,
                                 FlightMode flightMode,
                                 const std::string& theme)
    : Koopa(startX, startY, variant, theme), m_FlightMode(flightMode), m_BaseY(startY - TILE_SIZE) {
}

void KoopaParatroopa::Update(float deltaTime) {
    if (!m_HasWings) {
        Koopa::Update(deltaTime);
        return;
    }

    if (m_FlightMode == FlightMode::VerticalPatrol) {
        m_FlightTimer += deltaTime;
        m_Position.y = m_BaseY + std::sin(m_FlightTimer * 2.5f) * TILE_SIZE * 2.0f;
        m_Position.x += m_Velocity.x * deltaTime;
        UpdateDrawable();
        return;
    }

    if (m_Velocity.y == 0.0f) {
        m_Velocity.y = -250.0f;
    }
    Koopa::Update(deltaTime);
}

void KoopaParatroopa::Stomp() {
    if (m_HasWings) {
        m_HasWings = false;
        m_Velocity.y = 0.0f;
        LOG_INFO("KoopaParatroopa lost wings.");
        return;
    }

    Koopa::Stomp();
}
