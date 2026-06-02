#include "Player.hpp"

#include "GameConstants.hpp"

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "Core/Context.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "Util/Time.hpp"
#include "Util/ImagePath.hpp"

Player::Player() {
    m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    m_Position = {200.0f, 100.0f};
    m_PreviousPosition = m_Position;
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    SetZIndex(10.0f);

    // 建立三套動畫／圖片資源
    InitAnimations();

    // 預設顯示 Small Mario 待機圖（InitAnimations 之後才有值）
    SetDrawable(m_SmallIdleImage);
}

void Player::ResetForNewGame() {
    SetForm(Form::SMALL);
}

// ─── 初始化所有動畫資源 ───────────────────────────────────────────────

void Player::InitAnimations() {
    // ─ Small Mario ─
    m_SmallIdleImage = std::make_shared<Util::Image>(kSmallIdle);
    m_SmallJumpImage = std::make_shared<Util::Image>(kSmallJump);
    m_SmallWalkAnim  = std::make_shared<Util::Animation>(kSmallWalk, true, 120, true);
    m_SmallClimbAnim = std::make_shared<Util::Animation>(kSmallClimb, true, 150, true);
    m_SmallSkidImage = std::make_shared<Util::Image>(kSmallSkid);
    m_DeadImage      = std::make_shared<Util::Image>(kSmallDir + "Dead/Dead.png");

    // ─ Super Mario ─
    m_SuperIdleImage = std::make_shared<Util::Image>(kSuperIdle);
    m_SuperJumpImage = std::make_shared<Util::Image>(kSuperJump);
    m_SuperWalkAnim  = std::make_shared<Util::Animation>(kSuperWalk, true, 120, true);
    m_SuperClimbAnim = std::make_shared<Util::Animation>(kSuperClimb, true, 150, true);
    m_SuperSkidImage = std::make_shared<Util::Image>(kSuperSkid);

    // ─ Fire Mario ─
    m_FireIdleImage = std::make_shared<Util::Image>(kFireIdle);
    m_FireJumpImage = std::make_shared<Util::Image>(kFireJump);
    m_FireWalkAnim  = std::make_shared<Util::Animation>(kFireWalk, true, 120, true);
    m_FireClimbAnim = std::make_shared<Util::Animation>(kFireClimb, true, 150, true);
    m_FireSkidImage = std::make_shared<Util::Image>(kFireSkid);
    m_FireShootImage = std::make_shared<Util::Image>(kFireShoot);

    LOG_INFO("Player::InitAnimations done (Small / Super / Fire)");
}

// ─── 每幀主要更新 ────────────────────────────────────────────────────

void Player::Update(float deltaTime) {
    if (!m_IsAlive) return;
    //TODO 可以更多封裝
    m_PreviousPosition = m_Position;

    // ── 死亡動畫演出 ──
    if (m_IsDying) {
        SetVisible(true);
        m_DeathTimer += deltaTime;

        // 階段 1：前 0.5 秒畫面凍結（停在半空中）
        if (m_DeathTimer < 0.5f) {
            m_Velocity.y = 0.0f;
        }
        // 階段 2：0.5 秒瞬間，給一個往上的死亡彈跳
        else if (m_DeathTimer >= 0.5f && m_DeathTimer < 0.55f && m_Velocity.y == 0.0f) {
            m_Velocity.y = -500.0f;
        }
        // 階段 3：之後套用重力，讓瑪利歐掉出畫面外
        else {
            m_Velocity.y += m_Gravity * deltaTime;
            m_Position.y += m_Velocity.y * deltaTime;
        }
        return; // 死亡期間不執行一般邏輯
    }

    UpdateDamageInvincibility(deltaTime);
    UpdateStarInvincibility(deltaTime);

    if (m_ShootingTimer > 0.0f) {
        m_ShootingTimer -= deltaTime;
    }

    if (!m_IsLevelCleared) {
        // 1. 處理一般鍵盤輸入（更新速度 m_Velocity.x 與面朝方向）
        HandleInput(deltaTime);
        
        // 2. 根據速度更新水平位置
        m_Position.x += m_Velocity.x * deltaTime;

        // 3. 套用重力 (更新 m_Velocity.y 與 m_Position.y)
        ApplyGravity(deltaTime);
    } else {
        // 過關自動控制狀態
        if (m_IsSlidingDown) {
            // 沿著旗杆向下滑
            m_Position.y += m_Velocity.y * deltaTime;
            if (m_Position.y >= m_PoleBottomY - m_Size.y) {
                // 到底了，停止下滑，開始走向城堡
                m_Position.y = m_PoleBottomY - m_Size.y;
                m_Velocity.y = 0.0f;
                m_IsSlidingDown = false;
                m_IsWalkingToCastle = true;
                
                // 設定目標：往前走 6 格
                m_WalkTargetX = m_Position.x + 6.0f * TILE_SIZE;
                
                m_FacingLeft = false;
                m_Velocity.x = m_MaxWalkSpeed;
            }
        } else if (m_IsWalkingToCastle) {
            // 自動向右走
            m_Position.x += m_Velocity.x * deltaTime;
            ApplyGravity(deltaTime);
            
            if (m_Position.x >= m_WalkTargetX) {
                m_Position.x = m_WalkTargetX;
                m_Velocity.x = 0.0f;
                m_IsWalkingToCastle = false;
                SetVisible(false); // 消失
            }
        }
    }

    // 更新動畫
    UpdateAnimation();
}

void Player::UpdateDamageInvincibility(float deltaTime) {
    if (m_IsLevelCleared) {
        return;
    }

    if (m_DamageInvincibleTimer <= 0.0f) {
        SetVisible(true);
        return;
    }

    m_DamageInvincibleTimer -= deltaTime;
    if (m_DamageInvincibleTimer <= 0.0f) {
        m_DamageInvincibleTimer = 0.0f;
        m_DamageBlinkTimer = 0.0f;
        m_DamageBlinkVisible = true;
        SetVisible(true);
        return;
    }

    constexpr float BLINK_INTERVAL = 0.1f;
    m_DamageBlinkTimer += deltaTime;
    if (m_DamageBlinkTimer >= BLINK_INTERVAL) {
        m_DamageBlinkTimer = 0.0f;
        m_DamageBlinkVisible = !m_DamageBlinkVisible;
        SetVisible(m_DamageBlinkVisible);
    }
}

void Player::UpdateStarInvincibility(float deltaTime) {
    if (m_StarTimer <= 0.0f) return;

    m_StarTimer -= deltaTime;
    if (m_StarTimer < 0.0f) {
        m_StarTimer = 0.0f;
    }
}

// ─── 畫面同步 ────────────────────────────────────────────────────────

void Player::Draw(const Camera& camera) {
    if (!m_IsAlive) return;

    // 水平翻轉：面向左邊時把 scale.x 改成負數
    float scaleX = m_FacingLeft ? -GAME_SCALE : GAME_SCALE;
    m_Transform.scale = {scaleX, GAME_SCALE};

    glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);

    static float s_DebugAccumulator = 0.0f;
    s_DebugAccumulator += Util::Time::GetDeltaTimeMs() / 1000.0f;
    if (s_DebugAccumulator >= 1.0f) {
        s_DebugAccumulator = 0.0f;
        LOG_INFO( // debug Log
            "Player debug worldPos={} center={} screenPos={} size={} velocity={} onGround={} facingLeft={}",
            m_Position,
            centerPos,
            m_Transform.translation,
            m_Size,
            m_Velocity,
            m_OnGround,
            m_FacingLeft
        );
    }
}

// ─── 動畫切換邏輯 ────────────────────────────────────────────────────

void Player::UpdateAnimation() {
    // 小馬力歐 Y 身體只有一格，SUPER/FIRE 是兩格
    // SetForm() 已調整 m_Size，這裡只要切换圖片

    std::shared_ptr<Util::Image>     idleImg;
    std::shared_ptr<Util::Animation> walkAnim;
    std::shared_ptr<Util::Image>     jumpImg;
    std::shared_ptr<Util::Animation> climbAnim;
    std::shared_ptr<Util::Image>     skidImg;

    switch (m_Form) {
        case Form::SMALL:
            idleImg   = m_SmallIdleImage;
            walkAnim  = m_SmallWalkAnim;
            jumpImg   = m_SmallJumpImage;
            climbAnim = m_SmallClimbAnim;
            skidImg   = m_SmallSkidImage;
            break;
        case Form::SUPER:
            idleImg   = m_SuperIdleImage;
            walkAnim  = m_SuperWalkAnim;
            jumpImg   = m_SuperJumpImage;
            climbAnim = m_SuperClimbAnim;
            skidImg   = m_SuperSkidImage;
            break;
        case Form::FIRE:
            idleImg   = m_FireIdleImage;
            walkAnim  = m_FireWalkAnim;
            jumpImg   = m_FireJumpImage;
            climbAnim = m_FireClimbAnim;
            skidImg   = m_FireSkidImage;
            break;
    }

    if (m_IsLevelCleared) {
        if (m_IsSlidingDown) {
            SetDrawable(climbAnim);
        } else if (m_IsWalkingToCastle) {
            SetDrawable(walkAnim);
        } else {
            SetDrawable(idleImg);
        }
        return;
    }

    if (m_Form == Form::FIRE && m_ShootingTimer > 0.0f) {
        SetDrawable(m_FireShootImage);
        return;
    }

    if (!m_OnGround) {
        SetDrawable(jumpImg);
    } else if (m_IsSkidding) {
        SetDrawable(skidImg);
    } else if (m_IsMoving) {
        SetDrawable(walkAnim);
    } else {
        SetDrawable(idleImg);
    }
}

// ─── 鍵盤輸入 ────────────────────────────────────────────────────────

void Player::HandleInput(float deltaTime) {
    if (Util::Input::IsKeyDown(Util::Keycode::NUM_1)) {
        SetForm(Form::SMALL);
    } else if (Util::Input::IsKeyDown(Util::Keycode::NUM_2)) {
        SetForm(Form::SUPER);
    } else if (Util::Input::IsKeyDown(Util::Keycode::NUM_3)) {
        SetForm(Form::FIRE);
    }

    bool pressingLeft = Util::Input::IsKeyPressed(Util::Keycode::LEFT) || Util::Input::IsKeyPressed(Util::Keycode::A);
    bool pressingRight = Util::Input::IsKeyPressed(Util::Keycode::RIGHT) || Util::Input::IsKeyPressed(Util::Keycode::D);
    bool holdingRun = Util::Input::IsKeyPressed(Util::Keycode::Z);

    // 空中不能靠跑鍵加速，只有在地上才允許切換極速
    float maxSpeed;
    if (m_OnGround) {
        maxSpeed = holdingRun ? m_MaxRunSpeed : m_MaxWalkSpeed;
    } else {
        // 空中：極速維持起跳時的慣性（取現有絕對速度與走速中較大者）
        maxSpeed = std::max(std::abs(m_Velocity.x), m_MaxWalkSpeed);
    }

    m_IsSkidding = false; // 每幀重置，若達成條件再設為 true

    if (pressingLeft && !pressingRight) {
        m_FacingLeft = true;
        // 如果現在正在往右走（速度 > 0），套用更大的煞車加速度 (打滑)
        if (m_Velocity.x > 0.0f) {
            m_Velocity.x -= m_SkidAcceleration * deltaTime;
            // 只有在具備一定速度時反向才顯示打滑圖片
            if (m_Velocity.x > m_MaxWalkSpeed * 0.5f) {
                m_IsSkidding = true;
            }
        } else {
            const float accel = (m_OnGround && holdingRun) ? m_Acceleration * 1.5f : m_Acceleration;
            m_Velocity.x -= accel * deltaTime;
        }
    } else if (pressingRight && !pressingLeft) {
        m_FacingLeft = false;
        // 如果現在正在往左走（速度 < 0），套用更大的煞車加速度 (打滑)
        if (m_Velocity.x < 0.0f) {
            m_Velocity.x += m_SkidAcceleration * deltaTime;
            if (m_Velocity.x < -m_MaxWalkSpeed * 0.5f) {
                m_IsSkidding = true;
            }
        } else {
            const float accel = (m_OnGround && holdingRun) ? m_Acceleration * 1.5f : m_Acceleration;
            m_Velocity.x += accel * deltaTime;
        }
    } else {
        // 沒有按方向鍵，套用摩擦力減速
        if (m_Velocity.x > 0.0f) {
            m_Velocity.x -= m_Friction * deltaTime;
            if (m_Velocity.x < 0.0f) m_Velocity.x = 0.0f; // 完全停止
        } else if (m_Velocity.x < 0.0f) {
            m_Velocity.x += m_Friction * deltaTime;
            if (m_Velocity.x > 0.0f) m_Velocity.x = 0.0f; // 完全停止
        }
    }

    // 限制最高速度 (Clamp)
    if (m_Velocity.x > maxSpeed) {
        // 如果超過最高速度（例如剛放開跑鍵），讓他慢慢減速而不是瞬間掉速
        m_Velocity.x -= m_Friction * deltaTime;
        if (m_Velocity.x < maxSpeed) m_Velocity.x = maxSpeed;
    } else if (m_Velocity.x < -maxSpeed) {
        m_Velocity.x += m_Friction * deltaTime;
        if (m_Velocity.x > -maxSpeed) m_Velocity.x = -maxSpeed;
    }

    // 根據目前的絕對速度判斷是否真的有在移動 (給動畫播放判斷用)
    m_IsMoving = std::abs(m_Velocity.x) > 5.0f;

    // 跳躍（只有站在地上才能跳）
    if (m_OnGround &&
        (Util::Input::IsKeyDown(Util::Keycode::SPACE) ||
         Util::Input::IsKeyDown(Util::Keycode::UP) ||
         Util::Input::IsKeyDown(Util::Keycode::W))) {
        Jump();
    }

    // 射擊火球（只有 FIRE 形態可以，預設 Z 鍵）
    if (m_Form == Form::FIRE && Util::Input::IsKeyDown(Util::Keycode::Z)) {
        m_ShootRequested = true;
    }
}

void Player::Jump() {
    m_Velocity.y = -m_JumpStrength;
    m_OnGround = false;
}

// ─── 其他方法 ────────────────────────────────────────────────────────

void Player::SetSpawnPosition(glm::vec2 position) {
    m_Position = position;
    m_PreviousPosition = position;
    m_Velocity = {0.0f, 0.0f};
    m_IsAlive = true;
    m_IsDying = false;
    m_DeathTimer = 0.0f;
    m_IsLevelCleared = false;
    m_IsSlidingDown = false;
    m_IsWalkingToCastle = false;
    m_WalkTargetX = 0.0f;
    m_DamageInvincibleTimer = 0.0f;
    m_DamageBlinkTimer = 0.0f;
    m_DamageBlinkVisible = true;
    m_StarTimer = 0.0f;
    SetVisible(true);
}

void Player::ClampToCameraBounds(float cameraX) {
    // 不讓 Mario 跑到鏡頭左邊
    if (m_Position.x < cameraX) {
        m_Position.x = cameraX;
    }
}

// ─── SetForm：切換形態，同時調整碰撞體大小 ──────────────────────────

void Player::StartLevelClearSequence(float poleX, float bottomY) {
    m_IsLevelCleared = true;
    m_IsSlidingDown = true;
    m_IsWalkingToCastle = false;
    m_DamageInvincibleTimer = 0.0f;
    m_DamageBlinkTimer = 0.0f;
    m_DamageBlinkVisible = true;
    SetVisible(true);
    
    m_Position.x = poleX + TILE_SIZE * 0.5f; 
    
    m_PoleBottomY = bottomY;

    m_Velocity = {0.0f, 150.0f};
    
    m_FacingLeft = true;

    LOG_INFO("Player level clear sequence started. Target bottomY={}", bottomY);
}

void Player::StartDamageInvincibility(float duration) {
    if (duration <= 0.0f || m_IsDying || m_IsLevelCleared) return;

    m_DamageInvincibleTimer = duration;
    m_DamageBlinkTimer = 0.0f;
    m_DamageBlinkVisible = true;
    SetVisible(true);
}

void Player::ActivateStarInvincibility(float duration) {
    if (duration <= 0.0f || m_IsDying || m_IsLevelCleared) return;
    m_StarTimer = duration;
    SetVisible(true);
    LOG_INFO("Player star invincibility activated for {} seconds.", duration);
}

// ─── SetForm：切換形態，同時調整碰撞體大小 ──────────────────────────

void Player::SetForm(Form form) {
    m_Form = form;

    if (form == Form::SMALL) {
        // 小馬力歐：1 格高
        m_Size = {TILE_SIZE, TILE_SIZE};
    } else {
        // SUPER / FIRE：2 格高
        m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    }

    LOG_INFO("Player::SetForm -> {} | size={}", static_cast<int>(form), m_Size);
}

// ─── Downgrade：受傷降級 ──────────────────────────────────────────────

/*
 * 受傷時降一個等級：
 *   FIRE  → SUPER（不死，縮回大馬力歐）
 *   SUPER → SMALL（不死，縮小）
 *   SMALL → 死亡
 *
 * 原版 NES 降級後有短暫無敵時間，這裡留 TODO 給後續實作。
 */
void Player::Downgrade() {
    switch (m_Form) {
        case Form::FIRE:
            SetForm(Form::SUPER);
            LOG_INFO("Player downgraded: FIRE -> SUPER");
            // TODO: 觸發短暫無敵閃爍動畫
            break;
        case Form::SUPER:
            SetForm(Form::SMALL);
            LOG_INFO("Player downgraded: SUPER -> SMALL");
            // TODO: 觸發短暫無敵閃爍動畫
            break;
        case Form::SMALL:
            m_IsDying = true;
            m_DeathTimer = 0.0f;
            m_DamageInvincibleTimer = 0.0f;
            m_DamageBlinkTimer = 0.0f;
            m_DamageBlinkVisible = true;
            m_StarTimer = 0.0f;
            m_Velocity = {0.0f, 0.0f};
            SetDrawable(m_DeadImage);
            SetVisible(true);
            LOG_INFO("Player died! (SMALL hit by enemy) - Starting death animation");
            break;
    }
}
