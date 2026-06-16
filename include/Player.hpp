#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <array>
#include <memory>
#include <string>

#include "Character.hpp"
#include "ClipDrawable.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"
#include "Util/Keycode.hpp"
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
        IntroAutoWalk,
        EnteringPipe,
        ExitingPipe,
        Transforming  // 變身動畫中（吃道具升級或受傷縮小），凍結玩家物理與輸入
    };

    enum class VisualProfile {
        Mario,
        Luigi,
    };

    struct Controls {
        std::array<Util::Keycode, 3> left  = {Util::Keycode::LEFT,  Util::Keycode::A, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> right = {Util::Keycode::RIGHT, Util::Keycode::D, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> up    = {Util::Keycode::UP,    Util::Keycode::W, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> down  = {Util::Keycode::DOWN,  Util::Keycode::S, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> jump  = {Util::Keycode::SPACE, Util::Keycode::UP, Util::Keycode::W};
        std::array<Util::Keycode, 3> run   = {Util::Keycode::Z, Util::Keycode::UNKNOWN, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> fire  = {Util::Keycode::Z, Util::Keycode::UNKNOWN, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> debugSmall = {Util::Keycode::NUM_1, Util::Keycode::UNKNOWN, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> debugSuper = {Util::Keycode::NUM_2, Util::Keycode::UNKNOWN, Util::Keycode::UNKNOWN};
        std::array<Util::Keycode, 3> debugFire  = {Util::Keycode::NUM_3, Util::Keycode::UNKNOWN, Util::Keycode::UNKNOWN};
    };

    Player();
    void ResetForNewGame();
    void SetControls(const Controls& controls) { m_Controls = controls; }
    static Controls DefaultControls();
    static Controls PlayerTwoControls();
    void SetVisualProfile(VisualProfile profile);

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

    // ─── 變身動畫 ──────────────────────────────────────────────────────
    // 啟動升級變身動畫：從 fromForm 升到 toForm，全場凍結約 1 秒
    void StartTransformAnimation(Form fromForm, Form toForm);
    bool IsTransforming() const { return m_State == State::Transforming; }
    // 變身動畫完成後回傳最終形態
    Form GetTransformTargetForm() const { return m_TransformToForm; }

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

    // 給 AudioManager 詢問這幀是否剛跳躍（消耗式查詢）
    bool ConsumeJumpEvent() {
        if (m_JumpEventPending) {
            m_JumpEventPending = false;
            return true;
        }
        return false;
    }

    // 給 AudioManager 詢問星星無敵是否剛結束（消耗式查詢）
    // 結束後需切回關卡 BGM
    bool ConsumeStarEndedEvent() {
        if (m_StarEndedEventPending) {
            m_StarEndedEventPending = false;
            return true;
        }
        return false;
    }

    /* 設定出生點
     * 載入關卡時呼叫，重設位置並清除速度。
     */
    void SetSpawnPosition(glm::vec2 position);

    void SetOnGround(bool state);

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
        return m_IsLevelCleared && !m_IsSlidingDown && !m_IsWalkingToCastle
               && !m_IsEnteringDoor;
    }
    // 過關走位的城堡門中心 X（GameManager 用來定位城堡小旗）
    float GetCastleDoorX() const { return m_WalkTargetX; }

    void StartIntroAutoWalk(float targetX, float walkSpeed);
    bool IsIntroAutoWalkFinished() const { return m_IntroAutoWalkFinished; }

    // ─── 水管進出動畫方法 ───────────────────────────────────────────
    State GetState() const { return m_State; }
    void StartPipeEntry(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration = 1.0f);
    void StartPipeExit(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration = 1.0f);
    bool IsAnimationFinished() const { return m_AnimTimer >= m_AnimDuration; }

    bool IsPressingUp() const;
    bool IsPressingDown() const;
    bool IsPressingLeft() const;
    bool IsPressingRight() const;

    /* 鑽出水管動畫播完後恢復正常操作
     * 由 GameManager 在 ExitingPipe && IsAnimationFinished() 時呼叫，
     * 否則 ExitingPipe 永遠不會結束（玩家凍結在管口）。
     */
    void FinishPipeExit() {
        if (m_State != State::ExitingPipe) return;
        m_State = State::Normal;
        m_Velocity = {0.0f, 0.0f};
        m_OnGround = false; // 下一幀由方塊碰撞重新判定（通常正站在管口上）
    }

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
    void UpdateDamageFlicker(float deltaTime);
    void UpdateStarInvincibility(float deltaTime);
    void UpdateTransformAnimation(float deltaTime); // 變身閃爍動畫更新
    void ResetTransientState();

    struct VisualAssets {
        std::shared_ptr<Util::Image> idle;
        std::shared_ptr<Util::Animation> walk;
        std::shared_ptr<Util::Image> jump;
        std::shared_ptr<Util::Animation> climb;
        std::shared_ptr<Util::Image> skid;
        std::shared_ptr<Util::Image> duck;
        std::shared_ptr<Util::Image> shoot;
    };

    VisualAssets CreateVisualAssets(const std::string& dir, bool includeShoot);
    VisualAssets CreateLuigiVisualAssets(Form form);
    static std::size_t FormIndex(Form form);
    const VisualAssets& CurrentVisualAssets() const;
    static bool AnyPressed(const std::array<Util::Keycode, 3>& keys);
    static bool AnyDown(const std::array<Util::Keycode, 3>& keys);

    // ─── Player 專屬屬性 ───────────────────────────────────────────

    // ─── 物理與移動設定（速度常數見 GameConstants.hpp）─────────────
    glm::vec2 m_PreviousPosition = {0.0f, 0.0f};
    bool  m_OnGround         = false;  // 是否站在地上
    bool  m_JumpCut          = false;  // 跳躍中途放開跳鍵後鎖定為強重力

    bool  m_ShootRequested = false; // 是否要求發射火球
    float m_ShootingTimer  = 0.0f;  // 發射姿勢維持時間
    bool  m_JumpEventPending = false;   // 這幀是否剛跳躍（供 AudioManager 查詢）
    bool  m_StarEndedEventPending = false; // 星星效果剛結束（供 AudioManager 切回 BGM）

    bool  m_IsMoving   = false;    // 這幀有沒有按左右鍵（動畫用）
    bool  m_IsSkidding = false;    // 是否正在打滑
    bool  m_FacingLeft = false;    // 面向左邊嗎？（決定是否翻轉圖片）

    float m_DamageInvincibleTimer = 0.0f;
    float m_StarTimer = 0.0f;

    // ── 受傷閃爍（模擬 NES 逐幀顯示/隱藏 sprite，視覺上呈半透明）──
    float m_DamageFlickerTimer   = 0.0f;
    bool  m_DamageFlickerVisible = true;
    static constexpr float DAMAGE_FLICKER_INTERVAL = 1.0f / 30.0f; // 60fps 下每 2 幀切換

    Form m_Form = Form::SMALL;     // 預設小馬力歐
    static constexpr std::size_t FORM_COUNT = 3;
    std::array<VisualAssets, FORM_COUNT> m_NormalVisuals;
    std::array<VisualAssets, FORM_COUNT> m_StarVisuals;
    std::shared_ptr<Util::Image> m_DeadImage;
    Controls m_Controls;
    VisualProfile m_VisualProfile = VisualProfile::Mario;

    // ── 過關動畫狀態 ──
    bool  m_IsLevelCleared    = false;
    bool  m_IsSlidingDown     = false;
    bool  m_IsWalkingToCastle = false;
    bool  m_IsEnteringDoor    = false; // 走到城堡門口後沒入門內（最後階段）
    float m_PoleBottomY       = 0.0f;
    float m_WalkTargetX       = 0.0f; // 過關後自動行走的目標位置（城堡門中心）
    float m_DoorEnterX        = 0.0f; // 沒入門內的最終 X（門中心再往內一些）

    // ── 開場自動走路過場狀態 ──
    float m_IntroAutoWalkTargetX = 0.0f;
    float m_IntroAutoWalkSpeed = 0.0f;
    bool  m_IntroAutoWalkFinished = false;

    // ── 死亡動畫狀態 ──
    bool  m_IsDying = false;
    float m_DeathTimer = 0.0f;
    bool  m_DeathBounced = false;

    // ── 變身動畫狀態 ──
    Form  m_TransformFromForm = Form::SMALL; // 變身前形態
    Form  m_TransformToForm   = Form::SUPER; // 變身後形態
    bool  m_TransformIsDowngrade = false;    // 受傷縮小才閃爍可見性，吃道具升級不閃
    float m_TransformTimer    = 0.0f;        // 變身動畫累積時間
    float m_TransformBlinkTimer = 0.0f;      // 閃爍間隔計時器
    bool  m_TransformBlinkState = false;     // 目前顯示哪個形態（false=from, true=to）
    static constexpr float TRANSFORM_TOTAL_DURATION = 1.0f;   // 整體變身動畫時長（秒）
    static constexpr float TRANSFORM_BLINK_INTERVAL = 0.07f;  // 每次切換的間隔（秒）

    // ── 水管動畫狀態變數 ──
    State m_State = State::Normal;
    float m_AnimTimer = 0.0f;
    float m_AnimDuration = 1.0f;
    glm::vec2 m_AnimStartPos = {0.0f, 0.0f};
    glm::vec2 m_AnimEndPos = {0.0f, 0.0f};
    std::string m_PipeOpening = "up";
    glm::vec2 m_PipePos  = {0.0f, 0.0f}; // 進/出水管時的水管位置（左上角，世界座標）
    glm::vec2 m_PipeSize = {0.0f, 0.0f}; // 水管尺寸（世界座標）

    // 進/出水管時沿管口裁切馬力歐，做出「沒入管中」效果（水管只是背景圖、無法遮擋）。
    // 永久作為 GameObject 的 drawable；切換動畫時只更新其內層。
    std::shared_ptr<ClipDrawable> m_ClipWrapper;
    void SetVisual(const std::shared_ptr<Core::Drawable>& drawable);
};

#endif
