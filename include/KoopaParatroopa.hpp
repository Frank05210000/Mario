#ifndef KOOPA_PARATROOPA_HPP
#define KOOPA_PARATROOPA_HPP

#include "Koopa.hpp"

class KoopaParatroopa : public Koopa {
public:
    enum class FlightMode { Hop, VerticalPatrol };

    KoopaParatroopa(float startX,
                    float startY,
                    Variant variant = Variant::Red,
                    FlightMode flightMode = FlightMode::Hop);

    void Update(float deltaTime) override;
    void Stomp() override;

private:
    FlightMode m_FlightMode = FlightMode::Hop;
    bool m_HasWings = true;
    float m_BaseY = 0.0f;
    float m_FlightTimer = 0.0f;
};

#endif
