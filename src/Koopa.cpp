#include "Koopa.hpp"

#include <filesystem>
#include <vector>

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"
#include "Util/Logger.hpp"

Koopa::Koopa(float startX, float startY, Variant variant, const ThemeAssets& assets)
    : m_Variant(variant) {
    m_Position = {startX, startY - TILE_SIZE};
    m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    LoadSprites(assets);
    UpdateDrawable();
}

void Koopa::LoadSprites(const ThemeAssets& assets) {
    const std::string& theme = assets.Segment();
    const std::string variant = m_Variant == Variant::Red ? "red" : "normal";
    const std::string reverseVariant = m_Variant == Variant::Red ? "red_reverse" : "reverse";
    const std::string base = "enemy/Koopa/" + theme + "/";

    std::string walkDir = base + variant;
    std::string walkDirR = base + reverseVariant;
    if (!std::filesystem::exists(MakeAssetPath(walkDir + "/walk/walk-1.png"))) {
        walkDir = base + "normal";
        walkDirR = base + "reverse";
        LOG_WARN("Koopa: missing {} {} sprites; using normal fallback.", theme, variant);
    }

    m_WalkLeftAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(walkDir + "/walk/walk-1.png"),
            MakeAssetPath(walkDir + "/walk/walk-2.png"),
        },
        true, 160, true);
    m_WalkRightAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath(walkDirR + "/walk/walk-1.png"),
            MakeAssetPath(walkDirR + "/walk/walk-2.png"),
        },
        true, 160, true);

    std::string shellDir = base + variant + "/shell/";
    if (!std::filesystem::exists(MakeAssetPath(shellDir + "shell.png"))) {
        shellDir = base + "normal/shell/";
    }
    m_ShellImage = std::make_shared<Util::Image>(MakeAssetPath(shellDir + "shell.png"));

    const std::string revivePath = MakeAssetPath(shellDir + "shell_revive.png");
    m_ShellReviveImage = std::filesystem::exists(revivePath)
        ? std::make_shared<Util::Image>(revivePath)
        : m_ShellImage;

    const std::string flipPath = MakeAssetPath(shellDir + "shell_flip.png");
    m_ShellFlipImage = std::filesystem::exists(flipPath)
        ? std::make_shared<Util::Image>(flipPath)
        : m_ShellImage;
}

void Koopa::Update(float deltaTime) {
    if (!m_IsAlive) return;

    if (m_State == State::Defeated) {
        SetDrawable(m_ShellFlipImage);
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        return;
    }

    if (IsStationaryShell()) {
        ApplyGravity(deltaTime);
        m_ReviveTimer += deltaTime;
        const State timedState = StationaryShellStateForTimer(m_ReviveTimer);
        if (timedState == State::Walking) {
            EnterWalking();
        } else {
            m_State = timedState;
            SetDrawable(m_State == State::ShellReviving
                            ? m_ShellReviveImage
                            : m_ShellImage);
        }
        return;
    }

    if (m_State == State::ShellSliding) {
        SetDrawable(m_ShellImage);
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
        return;
    }

    UpdateDrawable();
    Enemy::Update(deltaTime);
}

Enemy::StompOutcome Koopa::Stomp() {
    const StompOutcome outcome = OutcomeForStomp(m_State);
    if (outcome == StompOutcome::NoEffect) return outcome;
    const State nextState = StateAfterStomp(m_State);
    EnterStationaryShell();
    m_State = nextState;
    return outcome;
}

void Koopa::EnterStationaryShell() {
    m_State = State::ShellIdle;
    m_ReviveTimer = 0.0f;
    m_ShellChainCount = 0;
    m_Velocity.x = 0.0f;
    SetDrawable(m_ShellImage);

    if (m_Size.y > TILE_SIZE) {
        m_Position.y += m_Size.y - TILE_SIZE;
        m_Size.y = TILE_SIZE;
    }
}

void Koopa::EnterWalking() {
    if (m_Size.y < TILE_SIZE * 2.0f) {
        m_Position.y -= TILE_SIZE * 2.0f - m_Size.y;
        m_Size.y = TILE_SIZE * 2.0f;
    }
    m_State = State::Walking;
    m_ReviveTimer = 0.0f;
    m_ShellChainCount = 0;
    m_Velocity.x = -m_WalkSpeed;
    UpdateDrawable();
}

void Koopa::Kick(bool kickLeft) {
    if (!IsStationaryShell()) return;
    m_State = State::ShellSliding;
    m_ReviveTimer = 0.0f;
    m_ShellChainCount = 0;
    m_Velocity.x = kickLeft ? -KOOPA_SHELL_SPEED : KOOPA_SHELL_SPEED;
    SetDrawable(m_ShellImage);
}

void Koopa::Die(bool flipLeft) {
    if (m_State == State::Defeated || !m_IsAlive) return;
    if (m_Size.y > TILE_SIZE) {
        m_Position.y += m_Size.y - TILE_SIZE;
        m_Size.y = TILE_SIZE;
    }
    m_State = State::Defeated;
    m_ReviveTimer = 0.0f;
    m_ShellChainCount = 0;
    m_IsGrounded = false;
    m_Velocity.y = -KOOPA_DEFEAT_Y_SPEED;
    m_Velocity.x = flipLeft ? -KOOPA_DEFEAT_X_SPEED : KOOPA_DEFEAT_X_SPEED;
    SetDrawable(m_ShellFlipImage);
    SetVisible(true);
}

void Koopa::Draw(const Camera& camera) {
    Enemy::Draw(camera);
}

void Koopa::UpdateDrawable() {
    if (m_State == State::ShellReviving) {
        SetDrawable(m_ShellReviveImage);
    } else if (IsInShell() || m_State == State::Defeated) {
        SetDrawable(m_State == State::Defeated ? m_ShellFlipImage : m_ShellImage);
    } else {
        SetDrawable(m_Velocity.x > 0.0f ? m_WalkRightAnim : m_WalkLeftAnim);
    }
}
