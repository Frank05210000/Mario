#ifndef KOOPA_HPP
#define KOOPA_HPP

#include <memory>

#include "Enemy.hpp"
#include "ThemeAssets.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

class Koopa : public Enemy {
public:
    enum class Variant { Green, Red };
    enum class State {
        Walking,
        ShellIdle,
        ShellReviving,
        ShellSliding,
        Defeated,
    };

    Koopa(float startX,
          float startY,
          Variant variant = Variant::Green,
          const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    StompOutcome Stomp() override;
    void Kick(bool kickLeft);
    void Die(bool flipLeft);
    void Draw(const Camera& camera) override;

    bool CanCollide() const override {
        return m_IsAlive && m_State != State::Defeated;
    }

    bool IsInShell() const {
        return m_State == State::ShellIdle ||
               m_State == State::ShellReviving ||
               m_State == State::ShellSliding;
    }
    bool IsStationaryShell() const {
        return m_State == State::ShellIdle || m_State == State::ShellReviving;
    }
    bool IsSliding() const { return m_State == State::ShellSliding; }
    bool IsDying() const { return m_State == State::Defeated; }
    bool IsWalking() const { return m_State == State::Walking; }
    State GetKoopaState() const { return m_State; }
    Variant GetVariant() const { return m_Variant; }

    static constexpr StompOutcome OutcomeForStomp(State state) {
        switch (state) {
            case State::Walking: return StompOutcome::EnteredShell;
            case State::ShellIdle:
            case State::ShellReviving:
            case State::ShellSliding: return StompOutcome::StoppedShell;
            case State::Defeated: return StompOutcome::NoEffect;
        }
        return StompOutcome::NoEffect;
    }

    static constexpr State StateAfterStomp(State state) {
        return state == State::Defeated ? State::Defeated : State::ShellIdle;
    }

    static constexpr State StationaryShellStateForTimer(float timer) {
        return timer >= KOOPA_REVIVE_DELAY
            ? State::Walking
            : (timer >= KOOPA_REVIVE_DELAY - KOOPA_REVIVE_WARNING
                   ? State::ShellReviving
                   : State::ShellIdle);
    }

    static constexpr bool KickLeftFromContact(float playerCenterX,
                                              float shellCenterX,
                                              bool playerFacingLeft) {
        constexpr float kCenterTieThreshold = 0.5f;
        if (playerCenterX < shellCenterX - kCenterTieThreshold) return false;
        if (playerCenterX > shellCenterX + kCenterTieThreshold) return true;
        return playerFacingLeft;
    }

    int ConsumeShellChainIndex() { return m_ShellChainCount++; }
    int GetShellChainCount() const { return m_ShellChainCount; }

protected:
    void UpdateDrawable();
    void LoadSprites(const ThemeAssets& assets);
    void EnterWalking();
    void EnterStationaryShell();
    void SetCollisionHeightPreservingBottom(float height);

    static constexpr float kKoopaWidth = TILE_SIZE;
    static constexpr float kStandingVisualHeight = TILE_SIZE * 2.0f;
    static constexpr float kStandingCollisionHeight = TILE_SIZE * 1.0f;
    static constexpr float kShellCollisionHeight = TILE_SIZE;

    std::shared_ptr<Util::Animation> m_WalkLeftAnim;
    std::shared_ptr<Util::Animation> m_WalkRightAnim;
    std::shared_ptr<Util::Image> m_ShellImage;
    std::shared_ptr<Util::Image> m_ShellReviveImage;
    std::shared_ptr<Util::Image> m_ShellFlipImage;

    State m_State = State::Walking;
    Variant m_Variant = Variant::Green;
    float m_ReviveTimer = 0.0f;
    int m_ShellChainCount = 0;
};

#endif
