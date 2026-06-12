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
    SetDrawable(m_NormalVisuals[FormIndex(Form::SMALL)].idle);
}

void Player::ResetForNewGame() {
    SetForm(Form::SMALL);
    ResetTransientState();
    UpdateAnimation();
    SetVisible(true);
}

// ─── 初始化所有動畫資源 ───────────────────────────────────────────────

void Player::InitAnimations() {
    const std::array<std::string, FORM_COUNT> formNames = {
        "Mario", "Super Mario", "Fiery Mario"
    };
    const std::array<std::string, FORM_COUNT> normalDirs = {
        kSmallDir, kSuperDir, kFireDir
    };

    for (std::size_t form = 0; form < FORM_COUNT; ++form) {
        const bool includeShoot = form == FormIndex(Form::FIRE);
        m_NormalVisuals[form] = CreateVisualAssets(normalDirs[form], includeShoot);
        m_DamageVisuals[form] = CreateVisualAssets(
            MakeAssetPath("player/Effects/Damage/" + formNames[form] + "/right/"),
            includeShoot);
        m_StarVisuals[form] = CreateVisualAssets(
            MakeAssetPath("player/Effects/Star1/" + formNames[form] + "/right/"),
            includeShoot);
    }

    m_DeadImage = std::make_shared<Util::Image>(kSmallDir + "Dead/Dead.png");

    LOG_INFO("Player::InitAnimations done (normal / damage / star palettes)");
}

Player::VisualAssets Player::CreateVisualAssets(const std::string& dir,
                                                 bool includeShoot) {
    VisualAssets assets;
    assets.idle = std::make_shared<Util::Image>(dir + "Walk1/Walk1.png");
    assets.walk = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            dir + "Walk1/Walk1-1.png",
            dir + "Walk1/Walk1-2.png",
            dir + "Walk1/Walk1-3.png",
        },
        true,
        120,
        true);
    assets.jump = std::make_shared<Util::Image>(dir + "Jump/Jump.png");
    assets.climb = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            dir + "Climb/Climb-1.png",
            dir + "Climb/Climb-2.png",
        },
        true,
        150,
        true);
    assets.skid = std::make_shared<Util::Image>(dir + "Skid/Skid.png");
    assets.duck = std::make_shared<Util::Image>(dir + "Duck/Duck.png");
    if (includeShoot) {
        assets.shoot = std::make_shared<Util::Image>(dir + "Shoot/Shoot.png");
    }
    return assets;
}

std::size_t Player::FormIndex(Form form) {
    return static_cast<std::size_t>(form);
}

const Player::VisualAssets& Player::CurrentVisualAssets() const {
    const std::size_t form = FormIndex(m_Form);

    if (m_DamageInvincibleTimer > 0.0f) {
        return m_DamageVisuals[form];
    }

    if (m_StarTimer > 0.0f) {
        return m_StarVisuals[form];
    }

    return m_NormalVisuals[form];
}

// ─── 每幀主要更新 ────────────────────────────────────────────────────

void Player::Update(float deltaTime) {
    if (!m_IsAlive) return;
    m_PreviousPosition = m_Position;

    switch (m_State) {
        case State::Normal: {
            UpdateDamageInvincibility(deltaTime);
            UpdateStarInvincibility(deltaTime);

            if (m_ShootingTimer > 0.0f) {
                m_ShootingTimer -= deltaTime;
            }

            // 1. 處理一般鍵盤輸入（更新速度 m_Velocity.x 與面朝方向）
            HandleInput(deltaTime);

            // 2. 根據速度更新水平位置
            m_Position.x += m_Velocity.x * deltaTime;

            // 3. 雙段重力：上升按住跳鍵用弱重力，放開或下降用強重力（防穿地終端速度 400）
            if (!m_OnGround) {
                bool jumpHeld = Util::Input::IsKeyPressed(Util::Keycode::SPACE)
                             || Util::Input::IsKeyPressed(Util::Keycode::UP)
                             || Util::Input::IsKeyPressed(Util::Keycode::W);
                if (!jumpHeld && m_Velocity.y < 0.0f) m_JumpCut = true;  // 放開即鎖定強重力
                float g = (m_Velocity.y < 0.0f && jumpHeld && !m_JumpCut)
                              ? GRAVITY_RISE
                              : GRAVITY_FALL;
                m_Velocity.y = std::min(m_Velocity.y + g * deltaTime, MAX_FALL_SPEED);
                m_Position.y += m_Velocity.y * deltaTime;
            }
            break;
        }

        case State::Dying: {
            SetVisible(true);
            m_DeathTimer += deltaTime;

            // 階段 1：前 0.5 秒畫面凍結（停在半空中）
            if (m_DeathTimer < 0.5f) {
                m_Velocity.y = 0.0f;
            }
            // 階段 2：0.5 秒時，給一個往上的死亡彈跳（只執行一次）
            else if (m_DeathTimer >= 0.5f && !m_DeathBounced) {
                m_Velocity.y = -500.0f;
                m_DeathBounced = true;
            }
            // 階段 3：之後套用重力，讓瑪利歐掉出畫面外
            else {
                m_Velocity.y += m_Gravity * deltaTime;
                m_Position.y += m_Velocity.y * deltaTime;
            }
            break;
        }

        case State::LevelClear: {
            UpdateDamageInvincibility(deltaTime);
            UpdateStarInvincibility(deltaTime);

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
                    m_Velocity.x = PLAYER_MAX_WALK_SPEED;
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
            break;
        }

        case State::EnteringPipe:
        case State::ExitingPipe: {
            m_AnimTimer += deltaTime;
            float ratio = std::min(m_AnimTimer / m_AnimDuration, 1.0f);
            m_Position = glm::mix(m_AnimStartPos, m_AnimEndPos, ratio);
            break;
        }

        case State::Transforming: {
            // 變身動畫：全場凍結（不更新物理），只跑閃爍
            UpdateTransformAnimation(deltaTime);
            break;
        }
    }

    // 更新動畫
    UpdateAnimation();
}

void Player::UpdateDamageInvincibility(float deltaTime) {
    if (m_State == State::LevelClear) {
        return;
    }

    if (m_DamageInvincibleTimer <= 0.0f) {
        return;
    }

    m_DamageInvincibleTimer -= deltaTime;
    if (m_DamageInvincibleTimer <= 0.0f) {
        m_DamageInvincibleTimer = 0.0f;
    }
}

void Player::UpdateStarInvincibility(float deltaTime) {
    if (m_StarTimer <= 0.0f) return;

    m_StarTimer -= deltaTime;
    if (m_StarTimer <= 0.0f) {
        m_StarTimer = 0.0f;
        m_StarEndedEventPending = true; // 通知 AudioManager 切回關卡 BGM
    }
}

// ─── 變身動畫 ─────────────────────────────────────────────────────────

void Player::StartTransformAnimation(Form fromForm, Form toForm) {
    m_State = State::Transforming;
    m_TransformFromForm = fromForm;
    m_TransformToForm   = toForm;
    m_TransformTimer    = 0.0f;
    m_TransformBlinkTimer = 0.0f;
    m_TransformBlinkState = false; // 從「舊形態」開始
    // 先切換到舊形態以便閃爍
    m_Form = fromForm;
    // 確保尺寸與舊形態一致（變身期間物理用舊尺寸）
    if (fromForm == Form::SMALL) {
        m_Size = {TILE_SIZE, TILE_SIZE};
    } else {
        m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    }
    SetVisible(true);
    LOG_INFO("Player transform animation started: {} -> {}",
             static_cast<int>(fromForm), static_cast<int>(toForm));
}

void Player::UpdateTransformAnimation(float deltaTime) {
    m_TransformTimer     += deltaTime;
    m_TransformBlinkTimer += deltaTime;

    // 每 TRANSFORM_BLINK_INTERVAL 秒切換一次顯示形態
    if (m_TransformBlinkTimer >= TRANSFORM_BLINK_INTERVAL) {
        m_TransformBlinkTimer = 0.0f;
        m_TransformBlinkState = !m_TransformBlinkState;
        // 切換 form（只切圖，尺寸動畫期間維持 fromForm）
        m_Form = m_TransformBlinkState ? m_TransformToForm : m_TransformFromForm;
        UpdateAnimation(); // 立即同步圖片
    }

    // 動畫結束：定格到新形態
    if (m_TransformTimer >= TRANSFORM_TOTAL_DURATION) {
        m_Form = m_TransformToForm;
        // 同步尺寸到新形態
        if (m_TransformToForm == Form::SMALL) {
            const float oldH = m_Size.y;
            m_Size = {TILE_SIZE, TILE_SIZE};
            m_Position.y += (oldH - m_Size.y);
        } else {
            const float oldH = m_Size.y;
            m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
            m_Position.y += (oldH - m_Size.y);
        }
        SetVisible(true);
        m_State = State::Normal;
        // 注意：升級的受傷無敵由 GameManager 在本幀偵測 IsTransforming() 結束後呼叫
        LOG_INFO("Player transform animation finished. Final form={}", static_cast<int>(m_Form));
        UpdateAnimation();
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
}

// ─── 動畫切換邏輯 ────────────────────────────────────────────────────

void Player::UpdateAnimation() {
    // 小馬力歐 Y 身體只有一格，SUPER/FIRE 是兩格
    // SetForm() 已調整 m_Size，這裡只要切换圖片

    const VisualAssets& visuals = CurrentVisualAssets();

    if (m_State == State::EnteringPipe || m_State == State::ExitingPipe) {
        if (m_PipeOpening == "up" || m_PipeOpening == "down") {
            SetDrawable(visuals.duck);
        } else {
            SetDrawable(visuals.walk);
        }
        return;
    }

    // 變身動畫中：顯示當前 m_Form 的待機圖（m_Form 由閃爍邏輯交替切換）
    if (m_State == State::Transforming) {
        SetDrawable(visuals.idle);
        return;
    }

    if (m_State == State::LevelClear) {
        if (m_IsSlidingDown) {
            SetDrawable(visuals.climb);
        } else if (m_IsWalkingToCastle) {
            SetDrawable(visuals.walk);
        } else {
            SetDrawable(visuals.idle);
        }
        return;
    }

    if (m_Form == Form::FIRE && m_ShootingTimer > 0.0f) {
        SetDrawable(visuals.shoot);
        return;
    }

    if (!m_OnGround) {
        SetDrawable(visuals.jump);
    } else if (m_IsSkidding) {
        SetDrawable(visuals.skid);
    } else if (m_IsMoving) {
        SetDrawable(visuals.walk);
    } else {
        SetDrawable(visuals.idle);
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

    // 空中保留慣性（取現有絕對速度與走速中較大者）；地面依跑鍵切換上限
    float maxSpeed;
    if (m_OnGround) {
        maxSpeed = holdingRun ? PLAYER_MAX_RUN_SPEED : PLAYER_MAX_WALK_SPEED;
    } else {
        // 空中：極速維持起跳時的慣性，不得用加速超過此值
        maxSpeed = std::max(std::abs(m_Velocity.x), PLAYER_MAX_WALK_SPEED);
    }

    m_IsSkidding = false; // 每幀重置，若達成條件再設為 true

    if (pressingLeft && !pressingRight) {
        m_FacingLeft = true;
        // 地面：往右走時套用打滑煞車；空中：直接加速（無打滑）
        if (m_OnGround && m_Velocity.x > 0.0f) {
            m_Velocity.x -= PLAYER_SKID_DECEL * deltaTime;
            // 只有在具備一定速度時反向才顯示打滑圖片
            if (m_Velocity.x > PLAYER_MAX_WALK_SPEED * 0.5f) {
                m_IsSkidding = true;
            }
        } else {
            const float accel = (m_OnGround && holdingRun)
                                    ? PLAYER_ACCELERATION * PLAYER_RUN_ACCEL_MULT
                                    : PLAYER_ACCELERATION;
            if (m_Velocity.x > -maxSpeed) {
                m_Velocity.x = std::max(m_Velocity.x - accel * deltaTime, -maxSpeed);
            }
        }
    } else if (pressingRight && !pressingLeft) {
        m_FacingLeft = false;
        // 地面：往左走時套用打滑煞車；空中：直接加速（無打滑）
        if (m_OnGround && m_Velocity.x < 0.0f) {
            m_Velocity.x += PLAYER_SKID_DECEL * deltaTime;
            if (m_Velocity.x < -PLAYER_MAX_WALK_SPEED * 0.5f) {
                m_IsSkidding = true;
            }
        } else {
            const float accel = (m_OnGround && holdingRun)
                                    ? PLAYER_ACCELERATION * PLAYER_RUN_ACCEL_MULT
                                    : PLAYER_ACCELERATION;
            if (m_Velocity.x < maxSpeed) {
                m_Velocity.x = std::min(m_Velocity.x + accel * deltaTime, maxSpeed);
            }
        }
    } else {
        // 沒有按方向鍵：地面套用摩擦力，空中保留慣性（vx 不衰減）
        if (m_OnGround) {
            if (m_Velocity.x > 0.0f) {
                m_Velocity.x -= PLAYER_FRICTION * deltaTime;
                if (m_Velocity.x < 0.0f) m_Velocity.x = 0.0f;
            } else if (m_Velocity.x < 0.0f) {
                m_Velocity.x += PLAYER_FRICTION * deltaTime;
                if (m_Velocity.x > 0.0f) m_Velocity.x = 0.0f;
            }
        }
    }

    // 限制最高速度（地面超速時慢慢衰減，空中不套摩擦）
    if (m_Velocity.x > maxSpeed) {
        if (m_OnGround) {
            m_Velocity.x -= PLAYER_FRICTION * deltaTime;
            if (m_Velocity.x < maxSpeed) m_Velocity.x = maxSpeed;
        } else {
            // 空中超速（放開跑鍵後）：直接夾到上限，保持慣性
            // maxSpeed 已是 std::max(|vx|, walk)，不會夾掉合法的跑速
        }
    } else if (m_Velocity.x < -maxSpeed) {
        if (m_OnGround) {
            m_Velocity.x += PLAYER_FRICTION * deltaTime;
            if (m_Velocity.x > -maxSpeed) m_Velocity.x = -maxSpeed;
        }
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
    float launch = PLAYER_JUMP_VELOCITY;                              // 250
    if (std::abs(m_Velocity.x) > PLAYER_RUN_JUMP_THRESHOLD)          // |vx| > 90
        launch += PLAYER_JUMP_RUN_BONUS;                              // +30 → 280
    m_Velocity.y = -launch;
    m_JumpCut = false;
    m_OnGround = false;
    m_JumpEventPending = true; // 通知 AudioManager 播跳躍音效
}

// ─── 其他方法 ────────────────────────────────────────────────────────

void Player::SetSpawnPosition(glm::vec2 position) {
    // 關卡 JSON 的出生點以「小馬力歐（16px 高）的左上角」為基準。
    // 這裡改以腳底對齊（bottom 固定在 y + TILE_SIZE），讓 SUPER/FIRE（32px 高）
    // 抵達時頭頂自動上移，而不是腳底嵌進地板 16px——嵌住後落地判定永遠
    // 不成立，會被側碰推擠並一路下沉到 killZ（換關開場即死的原因）。
    m_Position = {position.x, position.y + TILE_SIZE - m_Size.y};
    m_PreviousPosition = m_Position;
    ResetTransientState();
    UpdateAnimation();
    SetVisible(true);
}

void Player::SetOnGround(bool state) {
    m_OnGround = state;
    UpdateAnimation();
}

void Player::ResetTransientState() {
    m_Velocity = {0.0f, 0.0f};
    m_OnGround = false;
    m_JumpCut = false;
    m_IsMoving = false;
    m_IsSkidding = false;
    m_FacingLeft = false;
    m_ShootRequested = false;
    m_ShootingTimer = 0.0f;
    m_JumpEventPending = false;
    m_StarEndedEventPending = false;
    m_IsAlive = true;
    m_IsDying = false;
    m_DeathTimer = 0.0f;
    m_DeathBounced = false;
    m_IsLevelCleared = false;
    m_IsSlidingDown = false;
    m_IsWalkingToCastle = false;
    m_WalkTargetX = 0.0f;
    m_DamageInvincibleTimer = 0.0f;
    m_StarTimer = 0.0f;
    // 變身動畫狀態重置
    m_TransformTimer     = 0.0f;
    m_TransformBlinkTimer = 0.0f;
    m_TransformBlinkState = false;
    m_State = State::Normal; // 重置狀態為正常
}

void Player::ClampToCameraBounds(float cameraX) {
    // 碰撞盒左緣不得超出鏡頭左界，殺掉殘留左速度避免抵牆抖動
    if (m_Position.x < cameraX) {
        m_Position.x = cameraX;
        if (m_Velocity.x < 0.0f) m_Velocity.x = 0.0f;
    }
}

// ─── SetForm：切換形態，同時調整碰撞體大小 ──────────────────────────

void Player::StartLevelClearSequence(float poleX, float bottomY) {
    m_State = State::LevelClear;
    m_IsLevelCleared = true;
    m_IsSlidingDown = true;
    m_IsWalkingToCastle = false;
    m_DamageInvincibleTimer = 0.0f;
    SetVisible(true);
    
    m_Position.x = poleX + TILE_SIZE * 0.5f; 
    
    m_PoleBottomY = bottomY;

    m_Velocity = {0.0f, 150.0f};
    
    m_FacingLeft = true;

    LOG_INFO("Player level clear sequence started. Target bottomY={}", bottomY);
}

void Player::StartDamageInvincibility(float duration) {
    if (duration <= 0.0f || m_State == State::Dying || m_State == State::LevelClear) return;

    m_DamageInvincibleTimer = duration;
    SetVisible(true);
}

void Player::ActivateStarInvincibility(float duration) {
    if (duration <= 0.0f || m_State == State::Dying || m_State == State::LevelClear) return;
    m_StarTimer = duration;
    SetVisible(true);
    LOG_INFO("Player star invincibility activated for {} seconds.", duration);
}

void Player::SetForm(Form form) {
    m_Form = form;

    const float oldHeight = m_Size.y;

    if (form == Form::SMALL) {
        // 小馬力歐：1 格高
        m_Size = {TILE_SIZE, TILE_SIZE};
    } else {
        // SUPER / FIRE：2 格高
        m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    }

    // 形態切換時保持底部 Y 座標不變，避免瞬移。
    // 例：SUPER(32) → SMALL(16) 時，oldHeight - newHeight = +16，
    //     m_Position.y += 16 使底部維持在原地，Mario 不會懸空後跌落。
    // 反之 SMALL(16) → SUPER(32) 時 += -16，Mario 往上長大，腳不插地。
    const float newHeight = m_Size.y;
    m_Position.y += (oldHeight - newHeight);

    LOG_INFO("Player::SetForm -> {} | size={}", static_cast<int>(form), m_Size);
}

void Player::Downgrade() {
    switch (m_Form) {
        case Form::FIRE:
            // 觸發縮小變身動畫：FIRE -> SUPER（全場凍結 ~1 秒）
            StartTransformAnimation(Form::FIRE, Form::SUPER);
            LOG_INFO("Player downgraded: FIRE -> SUPER (transform animation)");
            break;
        case Form::SUPER:
            // 觸發縮小變身動畫：SUPER -> SMALL（全場凍結 ~1 秒）
            StartTransformAnimation(Form::SUPER, Form::SMALL);
            LOG_INFO("Player downgraded: SUPER -> SMALL (transform animation)");
            break;
        case Form::SMALL:
            m_State = State::Dying;
            m_IsDying = true;
            m_DeathTimer = 0.0f;
            m_DamageInvincibleTimer = 0.0f;
            m_StarTimer = 0.0f;
            m_Velocity = {0.0f, 0.0f};
            SetDrawable(m_DeadImage);
            SetVisible(true);
            LOG_INFO("Player died! (SMALL hit by enemy) - Starting death animation");
            break;
    }
}

// ─── 水管動畫專屬實作 ───────────────────────────────────────────────────

void Player::StartPipeEntry(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration) {
    m_State = State::EnteringPipe;
    m_PipeOpening = opening;
    m_AnimTimer = 0.0f;
    m_AnimDuration = duration;
    m_Velocity = {0.0f, 0.0f};

    m_AnimStartPos = m_Position;
    m_AnimEndPos = m_Position;

    if (opening == "up") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y - m_Size.y};
        m_AnimEndPos = {targetX, pipePosition.y};
    } else if (opening == "down") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y + pipeSize.y};
        m_AnimEndPos = {targetX, pipePosition.y + pipeSize.y - m_Size.y};
    } else if (opening == "left") {
        float targetY = pipePosition.y + pipeSize.y * 0.5f - m_Size.y * 0.5f;
        m_AnimStartPos = {pipePosition.x - m_Size.x, targetY};
        m_AnimEndPos = {pipePosition.x, targetY};
    } else if (opening == "right") {
        float targetY = pipePosition.y + pipeSize.y * 0.5f - m_Size.y * 0.5f;
        m_AnimStartPos = {pipePosition.x + pipeSize.x, targetY};
        m_AnimEndPos = {pipePosition.x + pipeSize.x - m_Size.x, targetY};
    }

    m_Position = m_AnimStartPos;
    LOG_INFO("Player pipe entry started: opening={} startPos={} endPos={}", opening, m_AnimStartPos, m_AnimEndPos);
}

void Player::StartPipeExit(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration) {
    m_State = State::ExitingPipe;
    m_PipeOpening = opening;
    m_AnimTimer = 0.0f;
    m_AnimDuration = duration;
    m_Velocity = {0.0f, 0.0f};

    if (opening == "up") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y};
        m_AnimEndPos = {targetX, pipePosition.y - m_Size.y};
    } else if (opening == "down") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y + pipeSize.y - m_Size.y};
        m_AnimEndPos = {targetX, pipePosition.y + pipeSize.y};
    } else if (opening == "left") {
        float targetY = pipePosition.y + pipeSize.y * 0.5f - m_Size.y * 0.5f;
        m_AnimStartPos = {pipePosition.x, targetY};
        m_AnimEndPos = {pipePosition.x - m_Size.x, targetY};
    } else if (opening == "right") {
        float targetY = pipePosition.y + pipeSize.y * 0.5f - m_Size.y * 0.5f;
        m_AnimStartPos = {pipePosition.x + pipeSize.x - m_Size.x, targetY};
        m_AnimEndPos = {pipePosition.x + pipeSize.x, targetY};
    }

    m_Position = m_AnimStartPos;
    LOG_INFO("Player pipe exit started: opening={} startPos={} endPos={}", opening, m_AnimStartPos, m_AnimEndPos);
}
