#include "PiranhaPlant.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Animation.hpp"

PiranhaPlant::PiranhaPlant(float extendedX, float extendedY) {
    m_Size      = {TILE_SIZE, TILE_SIZE * 1.5f};
    m_ExtendedY = extendedY;
    m_HiddenY   = extendedY + m_Size.y;
    m_Position  = {extendedX, m_HiddenY};
    m_PipeX     = extendedX + m_Size.x * 0.5f; // 水管中心 X
    m_Velocity  = {0.0f, 0.0f};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    // z-index 低於水管（水管預設約 1.0 ~ 2.0）才能隱藏進水管內
    // Enemy::Enemy() 預設設 5.0，這裡覆寫為 0.5（水管底層）
    SetZIndex(0.5f);

    std::vector<std::string> paths = {
        MakeAssetPath("enemy/Piranha/underground/piranha-1.png"),
        MakeAssetPath("enemy/Piranha/underground/piranha-2.png"),
    };
    m_Anim = std::make_shared<Util::Animation>(paths, true, 180, true);
    SetDrawable(m_Anim);
}

void PiranhaPlant::Update(float deltaTime) {
    if (!m_IsAlive) return;

    constexpr float MOVE_SPEED      = 24.0f;
    constexpr float HIDDEN_PAUSE    = 1.0f;
    constexpr float EXTENDED_PAUSE  = 1.2f;

    // 判斷玩家是否在水管正上方 / 附近
    const bool playerNear = std::abs(m_PlayerX - m_PipeX) <= PLAYER_NEAR_RANGE;

    switch (m_State) {
        case State::HiddenPause:
            m_Position.y = m_HiddenY;
            // 完全縮回時隱藏（避免在水管中微露）
            SetVisible(false);
            m_StateTimer += deltaTime;
            if (m_StateTimer >= HIDDEN_PAUSE) {
                // 玩家在附近時不升出（重置計時，繼續等待）
                if (playerNear) {
                    m_StateTimer = 0.0f;
                    break;
                }
                m_StateTimer = 0.0f;
                m_State      = State::Rising;
            }
            break;

        case State::Rising:
            // 升出途中若玩家靠近，立即縮回
            SetVisible(true);  // 開始升出時顯示
            if (playerNear) {
                m_StateTimer = 0.0f;
                m_State      = State::Lowering;
                break;
            }
            m_Position.y = std::max(m_ExtendedY, m_Position.y - MOVE_SPEED * deltaTime);
            if (m_Position.y <= m_ExtendedY) {
                m_Position.y = m_ExtendedY;
                m_StateTimer = 0.0f;
                m_State      = State::ExtendedPause;
            }
            break;

        case State::ExtendedPause:
            m_Position.y = m_ExtendedY;
            SetVisible(true);  // 升出後持續顯示
            m_StateTimer += deltaTime;
            if (m_StateTimer >= EXTENDED_PAUSE) {
                m_StateTimer = 0.0f;
                m_State      = State::Lowering;
            }
            break;

        case State::Lowering:
            // 縮回途中仍顯示（直到完全縮回）
            SetVisible(true);
            m_Position.y = std::min(m_HiddenY, m_Position.y + MOVE_SPEED * deltaTime);
            if (m_Position.y >= m_HiddenY) {
                m_Position.y = m_HiddenY;
                m_StateTimer = 0.0f;
                m_State      = State::HiddenPause;
            }
            break;
    }
}

void PiranhaPlant::Stomp() {
    // 食人花不可被踩（保持現有行為）
}
