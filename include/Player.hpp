#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <memory>

#include "Character.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

/*
 * Player：玩家控制的角色（馬力歐）
 *
 * 繼承 Character，實作鍵盤輸入、跳躍、座標換算、動畫切換。
 * Update() 每幀處理輸入與物理；Draw() 把位置同步到畫面。
 */
class Player : public Character {
public:

    /* 馬力歐的形態
     * SMALL：小馬力歐，碰到敵人直接死亡
     * SUPER：吃了蘑菇，變大、可以打磚塊
     * FIRE ：吃了火焰花，可以射火球
     */
    enum class Form { SMALL, SUPER, FIRE };

    enum class State {
        Normal,
        Dying,
        LevelClear,
        EnteringPipe,
        ExitingPipe
    };

    Player();
    void ResetForNewGame();

    // ─── 覆寫 Character 的純虛擬方法 ──────────────────────────────

    /* 每幀更新邏輯
     * 順序：1. 處理輸入  2. 套用重力  3. 暫時地板碰撞  4. 更新動畫
     */
    void Update(float deltaTime) override;

    /* 每幀同步顯示位置
     * 呼叫 camera.WorldToScreen() 更新 m_Transform。
     */
    void Draw(const Camera& camera) override;

    // ─── Player 專屬方法 ───────────────────────────────────────────

    // 取得當前形態
    Form GetForm() const { return m_Form; }
    void SetForm(Form form);

    // 受傷降級 (FIRE->SUPER->SMALL->死)
    void Downgrade();

    void StartDamageInvincibility(float duration = 2.0f);
    bool IsDamageInvincible() const { return m_DamageInvincibleTimer > 0.0f; }
    void ActivateStarInvincibility(float duration = 10.0f);
    bool IsStarInvincible() const { return m_StarTimer > 0.0f; }

    // 取得死亡狀態
    bool IsDying() const { return m_State == State::Dying; }

    bool IsFacingLeft() const { return m_FacingLeft; }
    bool IsOnGround() const { return m_OnGround; }
    glm::vec2 GetPreviousPosition() const { return m_PreviousPosition; }

    // 給 GameManager 詢問是否要發射火球
    bool ConsumeShootRequest() {
        if (m_ShootRequested) {
            m_ShootRequested = false;
            m_ShootingTimer = 0.1f; // 約 3 幀的時間 (0.1秒)
            return true;
        }
        return false;
    }

    /* 設定出生點
     * 載入關卡時呼叫，重設位置並清除速度。
     */
    void SetSpawnPosition(glm::vec2 position);

    void SetOnGround(bool state) { m_OnGround = state; }

    /* 限制不得超過鏡頭左邊
     * 防止馬力歐往左跑回已經過去的場景。
     */
    void ClampToCameraBounds(float cameraX);

    /* 
     * 啟動過關動畫序列
     * poleX: 旗杆的中心 X 座標
     * bottomY: 旗杆底部的 Y 座標 (滑行終點)
     */
    void StartLevelClearSequence(float poleX, float bottomY);
    bool IsLevelClearSequenceFinished() const {
        return m_IsLevelCleared && !m_IsSlidingDown && !m_IsWalkingToCastle;
    }

    // ─── 水管進出動畫方法 ───────────────────────────────────────────
    State GetState() const { return m_State; }
    void StartPipeEntry(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration = 1.0f);
    void StartPipeExit(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration = 1.0f);
    bool IsAnimationFinished() const { return m_AnimTimer >= m_AnimDuration; }

private:
    /* 處理鍵盤輸入
     * 左右：A/D 或方向鍵；跳躍：Space / W / ↑（需站在地上）。
     * 同時更新 m_IsMoving 和 m_FacingLeft 供動畫切換使用。
     */
    void HandleInput(float deltaTime);

    /* 執行跳躍
     * 給 m_Velocity.y 一個向上的初速，並標記離地。
     */
    void Jump();

    /* 切換動畫
     * 依照目前狀態（跳躍 / 走路 / 待機）選擇對應的動畫或圖片。
     * 在 Update() 最後呼叫。
     */
    void UpdateAnimation();

    /* 初始化所有動畫與圖片
     * 在建構子裡呼叫一次，建立 m_WalkAnim 等資源。
     */
    void InitAnimations();

    void UpdateDamageInvincibility(float deltaTime);
    void UpdateStarInvincibility(float deltaTime);
    void ResetTransientState();



    // ─── Player 專屬屬性 ───────────────────────────────────────────

    // ─── 物理與移動設定（速度常數見 GameConstants.hpp）─────────────
    glm::vec2 m_PreviousPosition = {0.0f, 0.0f};
    bool  m_OnGround         = false;  // 是否站在地上
    bool  m_JumpCut          = false;  // 跳躍中途放開跳鍵後鎖定為強重力

    bool  m_ShootRequested = false; // 是否要求發射火球
    float m_ShootingTimer  = 0.0f;  // 發射姿勢維持時間

    bool  m_IsMoving   = false;    // 這幀有沒有按左右鍵（動畫用）
    bool  m_IsSkidding = false;    // 是否正在打滑
    bool  m_FacingLeft = false;    // 面向左邊嗎？（決定是否翻轉圖片）

    float m_DamageInvincibleTimer = 0.0f;
    float m_DamageBlinkTimer = 0.0f;
    bool  m_DamageBlinkVisible = true;
    float m_StarTimer = 0.0f;
    float m_StarBlinkTimer = 0.0f;   // 星星閃爍計時器
    bool  m_StarBlinkVisible = true; // 星星閃爍時的顯示狀態

    Form m_Form = Form::SMALL;     // 預設小馬力歐
    // ─── 動畫資源（三套形態，right 方向；Draw() 裡用 scaleX 翻轉） ──

    // SMALL Mario
    std::shared_ptr<Util::Image>     m_SmallIdleImage;
    std::shared_ptr<Util::Animation> m_SmallWalkAnim;
    std::shared_ptr<Util::Image>     m_SmallJumpImage;
    std::shared_ptr<Util::Animation> m_SmallClimbAnim;
    std::shared_ptr<Util::Image>     m_SmallSkidImage;
    std::shared_ptr<Util::Image>     m_SmallDuckImage;
    std::shared_ptr<Util::Image>     m_DeadImage;      // 瑪利歐死亡專用圖片

    // SUPER Mario
    std::shared_ptr<Util::Image>     m_SuperIdleImage;
    std::shared_ptr<Util::Animation> m_SuperWalkAnim;
    std::shared_ptr<Util::Image>     m_SuperJumpImage;
    std::shared_ptr<Util::Animation> m_SuperClimbAnim;
    std::shared_ptr<Util::Image>     m_SuperSkidImage;
    std::shared_ptr<Util::Image>     m_SuperDuckImage;

    // FIRE Mario
    std::shared_ptr<Util::Image>     m_FireIdleImage;
    std::shared_ptr<Util::Animation> m_FireWalkAnim;
    std::shared_ptr<Util::Image>     m_FireJumpImage;
    std::shared_ptr<Util::Animation> m_FireClimbAnim;
    std::shared_ptr<Util::Image>     m_FireSkidImage;
    std::shared_ptr<Util::Image>     m_FireShootImage;
    std::shared_ptr<Util::Image>     m_FireDuckImage;

    // ── 過關動畫狀態 ──
    bool  m_IsLevelCleared    = false;
    bool  m_IsSlidingDown     = false;
    bool  m_IsWalkingToCastle = false;
    float m_PoleBottomY       = 0.0f;
    float m_WalkTargetX       = 0.0f; // 過關後自動行走的目標位置

    // ── 死亡動畫狀態 ──
    bool  m_IsDying = false;
    float m_DeathTimer = 0.0f;
    bool  m_DeathBounced = false;

    // ── 水管動畫狀態變數 ──
    State m_State = State::Normal;
    float m_AnimTimer = 0.0f;
    float m_AnimDuration = 1.0f;
    glm::vec2 m_AnimStartPos = {0.0f, 0.0f};
    glm::vec2 m_AnimEndPos = {0.0f, 0.0f};
    std::string m_PipeOpening = "up";
};

#endif
