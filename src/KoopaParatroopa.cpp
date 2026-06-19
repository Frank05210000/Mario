#include "KoopaParatroopa.hpp"

#include <cmath>
#include <filesystem>
#include <vector>

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Logger.hpp"

KoopaParatroopa::KoopaParatroopa(float startX,
                                 float startY,
                                 Variant variant,
                                 FlightMode flightMode,
                                 const ThemeAssets& assets)
    : Koopa(startX, startY, variant, assets),
      m_FlightMode(flightMode) {
    m_BaseY = m_Position.y;
    LoadFlightSprites(assets);
    UpdateFlightDrawable();
}

void KoopaParatroopa::LoadFlightSprites(const ThemeAssets& assets) {
    const std::string base = "enemy/Koopa/" + assets.Segment() + "/";
    const std::string variant = m_Variant == Variant::Red ? "red" : "normal";
    const std::string reverseVariant = m_Variant == Variant::Red ? "red_reverse" : "reverse";
    std::string flyDir = base + variant + "/fly/";
    std::string flyDirR = base + reverseVariant + "/fly/";

    if (!std::filesystem::exists(MakeAssetPath(flyDir + "fly-1.png"))) {
        LOG_WARN("KoopaParatroopa: missing flight sprites; using walk fallback.");
        m_FlyLeftAnim = m_WalkLeftAnim;
        m_FlyRightAnim = m_WalkRightAnim;
        return;
    }

    m_FlyLeftAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(flyDir + "fly-1.png"),
            MakeAssetPath(flyDir + "fly-2.png"),
        },
        true, 120, true);
    m_FlyRightAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(flyDirR + "fly-1.png"),
            MakeAssetPath(flyDirR + "fly-2.png"),
        },
        true, 120, true);
}

void KoopaParatroopa::Update(float deltaTime) {
    if (!m_HasWings) {
        Koopa::Update(deltaTime);
        return;
    }

    if (m_FlightMode == FlightMode::VerticalPatrol) {
        m_FlightTimer += deltaTime;
        m_Position.y = m_BaseY + std::sin(m_FlightTimer * 2.5f) * TILE_SIZE * 2.0f;
        m_Velocity = {0.0f, 0.0f};
        UpdateFlightDrawable();
        return;
    }

    if (IsGrounded()) {
        m_Velocity.y = -PARATROOPA_JUMP_SPEED;
        SetGrounded(false);
    }
    Enemy::Update(deltaTime);
    UpdateFlightDrawable();
}

Enemy::StompOutcome KoopaParatroopa::Stomp() {
    if (m_HasWings) {
        m_HasWings = false;
        m_FlightTimer = 0.0f;
        if (m_Velocity.x == 0.0f) {
            m_Velocity.x = -m_WalkSpeed;
        }
        m_Velocity.y = 0.0f;
        SetGrounded(false);
        UpdateDrawable();
        LOG_INFO("KoopaParatroopa lost wings.");
        return StompOutcome::LostWings;
    }

    return Koopa::Stomp();
}

void KoopaParatroopa::Die(bool flipLeft) {
    m_HasWings = false;        // 先掉翅膀，Update() 才會走 Koopa 的 Defeated 死亡邏輯
    Koopa::Die(flipLeft);
}

void KoopaParatroopa::UpdateFlightDrawable() {
    SetDrawable(m_Velocity.x > 0.0f ? m_FlyRightAnim : m_FlyLeftAnim);
}
