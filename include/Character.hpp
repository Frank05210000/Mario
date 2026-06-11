#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "GameConstants.hpp"
#include "Util/GameObject.hpp"

class Camera; // 前置宣告


/*
 * Character：所有角色的抽象基底類別
 *
 * 繼承 Util::GameObject，代表 Character 本身就是可以顯示在螢幕上的物件。
 * Player 和 Enemy 都繼承 Character，共用物理屬性（位置、速度、重力）。
 *
 * 純虛擬方法 Update() 和 Draw() 強制子類別各自實作，
 * 因為 Player 靠鍵盤輸入，Enemy 靠 AI，行為完全不同。
 */
class Character : public Util::GameObject {
public:
    Character() = default;
    virtual ~Character() = default;

    // ─── 純虛擬方法（子類別必須實作）─────────────────────────────

    /* 每幀更新角色邏輯
     * 子類別負責處理：移動、輸入、AI、動畫切換等。
     * deltaTime：這幀花了多少秒，用來讓速度不受幀率影響。
     */
    virtual void Update(float deltaTime) = 0;

    /* 每幀更新角色的顯示位置
     * 把遊戲世界座標換算成螢幕座標 (套用 GAME_SCALE)
     */
    virtual void Draw(const Camera& camera) = 0;

    // ─── 共用方法（Player 和 Enemy 都適用）────────────────────────

    /* 套用重力
     * 每幀讓 m_Velocity.y 增加，模擬自由落體。
     * 需要在 Update() 裡呼叫。
     */
    void ApplyGravity(float deltaTime);

    // 取得/設定位置
    glm::vec2 GetPosition() const { return m_Position; }
    void SetPosition(glm::vec2 pos) { m_Position = pos; }

    // 取得/設定速度
    glm::vec2 GetVelocity() const { return m_Velocity; }
    void SetVelocity(glm::vec2 vel) { m_Velocity = vel; }

    // 取得大小（碰撞偵測用）
    glm::vec2 GetSize() const { return m_Size; }

    // 查詢 / 設定存活狀態
    bool IsAlive() const { return m_IsAlive; }
    void SetAlive(bool alive) { m_IsAlive = alive; }

protected:
    // ─── 共用屬性（子類別可直接存取）─────────────────────────────

    glm::vec2 m_Position = {0.0f, 0.0f}; // 世界座標位置
    glm::vec2 m_Velocity = {0.0f, 0.0f}; // 移動速度（像素/秒）
    glm::vec2 m_Size     = {0.0f, 0.0f}; // 寬高（碰撞盒大小）

    bool  m_IsAlive = true;    // 死亡時停止更新與顯示
    float m_Gravity = ENEMY_GRAVITY;  // 重力加速度（敵人與死亡動畫用，700 px/s²）
};

#endif
