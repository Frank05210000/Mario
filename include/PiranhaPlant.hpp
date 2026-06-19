#ifndef PIRANHA_PLANT_HPP
#define PIRANHA_PLANT_HPP

#include <memory>

#include "Enemy.hpp"
#include "ClipDrawable.hpp"
#include "Util/Animation.hpp"

class PiranhaPlant : public Enemy {
public:
    PiranhaPlant(float extendedX, float extendedY);

    void Update(float deltaTime) override;
    void Draw(const Camera& camera) override;
    StompOutcome Stomp() override;
    bool UsesBlockCollision() const override { return false; }

    /* 設定玩家的位置與尺寸（每幀由 GameManager 呼叫）
     * 若玩家站在水管正上方，植物不升出。
     */
    void SetPlayerBounds(glm::vec2 playerPosition, glm::vec2 playerSize) {
        m_PlayerPosition = playerPosition;
        m_PlayerSize = playerSize;
    }

    /* 查詢是否隱藏中（藏在管內，不對火球發生碰撞） */
    bool IsHidden() const { return m_State == State::HiddenPause; }

private:
    enum class State {
        HiddenPause,
        Rising,
        ExtendedPause,
        Lowering,
    };

    bool IsPlayerStandingOnPipeMouth() const;

    // 玩家 X 在管口附近的判定半徑（世界像素）
    static constexpr float PLAYER_NEAR_RANGE = TILE_SIZE * 1.5f;
    static constexpr float PLAYER_STANDING_Y_TOLERANCE = TILE_SIZE * 0.5f;

    float m_ExtendedY   = 0.0f;
    float m_HiddenY     = 0.0f;
    float m_StateTimer  = 0.0f;
    float m_PipeX       = 0.0f;   // 水管中心 X（用來判斷玩家是否在上方）
    glm::vec2 m_PlayerPosition = {-9999.0f, -9999.0f};
    glm::vec2 m_PlayerSize = {0.0f, 0.0f};
    State m_State       = State::HiddenPause;

    std::shared_ptr<Util::Animation> m_Anim;
    std::shared_ptr<ClipDrawable>    m_Clip;
};

#endif
