#ifndef PIRANHA_PLANT_HPP
#define PIRANHA_PLANT_HPP

#include <memory>

#include "Enemy.hpp"
#include "Util/Animation.hpp"

class PiranhaPlant : public Enemy {
public:
    PiranhaPlant(float extendedX, float extendedY);

    void Update(float deltaTime) override;
    StompOutcome Stomp() override;
    bool UsesBlockCollision() const override { return false; }

    /* 設定玩家的 X 座標（每幀由 GameManager 呼叫）
     * 若玩家在水管正上方 / 附近，植物不升出。
     */
    void SetPlayerX(float playerX) { m_PlayerX = playerX; }

    /* 查詢是否隱藏中（藏在管內，不對火球發生碰撞） */
    bool IsHidden() const { return m_State == State::HiddenPause; }

private:
    enum class State {
        HiddenPause,
        Rising,
        ExtendedPause,
        Lowering,
    };

    // 玩家 X 在管口附近的判定半徑（世界像素）
    static constexpr float PLAYER_NEAR_RANGE = TILE_SIZE * 1.5f;

    float m_ExtendedY   = 0.0f;
    float m_HiddenY     = 0.0f;
    float m_StateTimer  = 0.0f;
    float m_PipeX       = 0.0f;   // 水管中心 X（用來判斷玩家是否在上方）
    float m_PlayerX     = -9999.0f; // 玩家的世界 X 座標
    State m_State       = State::HiddenPause;

    std::shared_ptr<Util::Animation> m_Anim;
};

#endif
