#include "PiranhaPlant.hpp"

#include <algorithm>
#include <vector>

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Animation.hpp"

PiranhaPlant::PiranhaPlant(float extendedX, float extendedY) {
    m_Size = {TILE_SIZE, TILE_SIZE * 1.5f};
    m_ExtendedY = extendedY;
    m_HiddenY = extendedY + m_Size.y;
    m_Position = {extendedX, m_HiddenY};
    m_Velocity = {0.0f, 0.0f};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    SetZIndex(6.0f);

    std::vector<std::string> paths = {
        MakeAssetPath("enemy/Piranha/underground/piranha-1.png"),
        MakeAssetPath("enemy/Piranha/underground/piranha-2.png"),
    };
    m_Anim = std::make_shared<Util::Animation>(paths, true, 180, true);
    SetDrawable(m_Anim);
}

void PiranhaPlant::Update(float deltaTime) {
    if (!m_IsAlive) return;

    constexpr float MOVE_SPEED = 24.0f;
    constexpr float HIDDEN_PAUSE = 1.0f;
    constexpr float EXTENDED_PAUSE = 1.2f;

    switch (m_State) {
        case State::HiddenPause:
            m_Position.y = m_HiddenY;
            m_StateTimer += deltaTime;
            if (m_StateTimer >= HIDDEN_PAUSE) {
                m_StateTimer = 0.0f;
                m_State = State::Rising;
            }
            break;

        case State::Rising:
            m_Position.y = std::max(m_ExtendedY, m_Position.y - MOVE_SPEED * deltaTime);
            if (m_Position.y <= m_ExtendedY) {
                m_Position.y = m_ExtendedY;
                m_StateTimer = 0.0f;
                m_State = State::ExtendedPause;
            }
            break;

        case State::ExtendedPause:
            m_Position.y = m_ExtendedY;
            m_StateTimer += deltaTime;
            if (m_StateTimer >= EXTENDED_PAUSE) {
                m_StateTimer = 0.0f;
                m_State = State::Lowering;
            }
            break;

        case State::Lowering:
            m_Position.y = std::min(m_HiddenY, m_Position.y + MOVE_SPEED * deltaTime);
            if (m_Position.y >= m_HiddenY) {
                m_Position.y = m_HiddenY;
                m_StateTimer = 0.0f;
                m_State = State::HiddenPause;
            }
            break;
    }
}

void PiranhaPlant::Stomp() {
    // Piranha Plants are hazards, not stompable walking enemies.
}
