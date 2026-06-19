#include "PiranhaPlant.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "ClipDrawable.hpp"
#include "GameConstants.hpp"
#include "Core/Context.hpp"
#include "Util/Animation.hpp"

PiranhaPlant::PiranhaPlant(float extendedX, float extendedY) {
    m_Size      = {TILE_SIZE, TILE_SIZE * 1.5f};
    m_ExtendedY = extendedY;
    m_HiddenY   = extendedY + m_Size.y;
    m_Position  = {extendedX, m_HiddenY};
    m_PipeX     = extendedX + m_Size.x * 0.5f; // 水管中心 X
    m_Velocity  = {0.0f, 0.0f};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    SetZIndex(0.5f);

    std::vector<std::string> paths = {
        MakeAssetPath("enemy/Piranha/underground/piranha-1.png"),
        MakeAssetPath("enemy/Piranha/underground/piranha-2.png"),
    };
    m_Anim = std::make_shared<Util::Animation>(paths, true, 180, true);

    // 水管只是背景圖（z=-1），無法遮住食人花，導致看起來「透明」。
    // 改用裁切：只露出管口（m_HiddenY，植物完全縮回時的頂線）以上的部分。
    m_Clip = std::make_shared<ClipDrawable>(m_Anim);
    SetDrawable(m_Clip);
}

void PiranhaPlant::Draw(const Camera& camera) {
    Enemy::Draw(camera); // 設定 transform 與可見性

    if (!m_Clip) return;
    const auto  ctx   = Core::Context::GetInstance();
    const float halfH = static_cast<float>(ctx->GetWindowHeight()) * 0.5f;
    // 管口 framebuffer 像素 Y：保留此線以上（食人花露出的部分）
    const float mouthPx =
        camera.WorldToScreen({m_Position.x, m_HiddenY}).y + halfH;
    m_Clip->SetClipKeepAbovePx(mouthPx);
    m_Clip->SetEnabled(true);
}

void PiranhaPlant::Update(float deltaTime) {
    if (!m_IsAlive) return;

    constexpr float MOVE_SPEED      = 24.0f;
    constexpr float HIDDEN_PAUSE    = 1.0f;
    constexpr float EXTENDED_PAUSE  = 1.2f;

    const bool playerStandingOnPipe = IsPlayerStandingOnPipeMouth();

    switch (m_State) {
        case State::HiddenPause:
            m_Position.y = m_HiddenY;
            // 完全縮回時隱藏（避免在水管中微露）
            SetVisible(false);
            m_StateTimer += deltaTime;
            if (m_StateTimer >= HIDDEN_PAUSE) {
                // 玩家站在管口時不升出（重置計時，繼續等待）
                if (playerStandingOnPipe) {
                    m_StateTimer = 0.0f;
                    break;
                }
                m_StateTimer = 0.0f;
                m_State      = State::Rising;
            }
            break;

        case State::Rising:
            SetVisible(true);  // 開始升出時顯示
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

bool PiranhaPlant::IsPlayerStandingOnPipeMouth() const {
    const float playerCenterX = m_PlayerPosition.x + m_PlayerSize.x * 0.5f;
    const float playerBottom = m_PlayerPosition.y + m_PlayerSize.y;
    const bool horizontallyOverMouth =
        std::abs(playerCenterX - m_PipeX) <= PLAYER_NEAR_RANGE;
    const bool standingAtMouthHeight =
        std::abs(playerBottom - m_HiddenY) <= PLAYER_STANDING_Y_TOLERANCE;
    return horizontallyOverMouth && standingAtMouthHeight;
}

Enemy::StompOutcome PiranhaPlant::Stomp() {
    // 食人花不可被踩（保持現有行為）
    return StompOutcome::NoEffect;
}
