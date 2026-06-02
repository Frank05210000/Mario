#ifndef PIRANHA_PLANT_HPP
#define PIRANHA_PLANT_HPP

#include <memory>

#include "Enemy.hpp"
#include "Util/Animation.hpp"

class PiranhaPlant : public Enemy {
public:
    PiranhaPlant(float extendedX, float extendedY);

    void Update(float deltaTime) override;
    void Stomp() override;
    bool UsesBlockCollision() const override { return false; }

private:
    enum class State {
        HiddenPause,
        Rising,
        ExtendedPause,
        Lowering,
    };

    float m_ExtendedY = 0.0f;
    float m_HiddenY = 0.0f;
    float m_StateTimer = 0.0f;
    State m_State = State::HiddenPause;

    std::shared_ptr<Util::Animation> m_Anim;
};

#endif
