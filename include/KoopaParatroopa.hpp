#ifndef KOOPA_PARATROOPA_HPP
#define KOOPA_PARATROOPA_HPP

#include "Koopa.hpp"

class KoopaParatroopa : public Koopa {
public:
    enum class FlightMode { Hop, VerticalPatrol };

    KoopaParatroopa(float startX,
                    float startY,
                    Variant variant = Variant::Red,
                    FlightMode flightMode = FlightMode::Hop,
                    const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    void Update(float deltaTime) override;
    StompOutcome Stomp() override;
    bool UsesBlockCollision() const override {
        return !m_HasWings || m_FlightMode == FlightMode::Hop;
    }

    bool HasWings() const { return m_HasWings; }

private:
    void LoadFlightSprites(const ThemeAssets& assets);
    void UpdateFlightDrawable();

    FlightMode m_FlightMode = FlightMode::Hop;
    bool m_HasWings = true;
    float m_BaseY = 0.0f;
    float m_FlightTimer = 0.0f;
    std::shared_ptr<Util::Animation> m_FlyLeftAnim;
    std::shared_ptr<Util::Animation> m_FlyRightAnim;
};

#endif
