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

#include <algorithm>

namespace {
constexpr const char* kLuigiSmallDir = "player/LuigiSprites/native/small_luigi/";
constexpr const char* kLuigiBigDir = "player/LuigiSprites/native/big_luigi/";
constexpr const char* kLuigiFireDir = "player/LuigiSprites/native/fire_luigi/";
}

Player::Player() {
    m_Size = {TILE_SIZE, TILE_SIZE * 2.0f};
    m_Position = {200.0f, 100.0f};
    m_PreviousPosition = m_Position;
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    SetZIndex(10.0f);

    // 建立三套動畫／圖片資源
    InitAnimations();

    // 預設顯示 Small Mario 待機圖（InitAnimations 之後才有值）
    // 永久套一層 ClipDrawable，進/出水管時沿管口裁切；平時停用裁切。
    m_ClipWrapper = std::make_shared<ClipDrawable>(
        m_NormalVisuals[FormIndex(Form::SMALL)].idle);
    SetDrawable(m_ClipWrapper);
}

void Player::SetVisual(const std::shared_ptr<Core::Drawable>& drawable) {
    // 只替換 ClipDrawable 的內層，外層包裝（含裁切設定）維持不變。
    m_ClipWrapper->SetInner(drawable);
}

void Player::ResetForNewGame() {
    SetForm(Form::SMALL);
    ResetTransientState();
    UpdateAnimation();
    SetVisible(true);
}

// ─── 初始化所有動畫資源 ───────────────────────────────────────────────

void Player::InitAnimations() {
    if (m_VisualProfile == VisualProfile::Luigi) {
        for (std::size_t form = 0; form < FORM_COUNT; ++form) {
            m_NormalVisuals[form] = CreateLuigiVisualAssets(static_cast<Form>(form));
            for (std::size_t palette = 0; palette < STAR_PALETTE_COUNT; ++palette) {
                m_StarVisuals[palette][form] = m_NormalVisuals[form];
            }
        }
        m_DeadImage = std::make_shared<Util::Image>(
            MakeAssetPath(std::string(kLuigiSmallDir) + "frame_14_8x8.png"));
        LOG_INFO("Player::InitAnimations done (Luigi visuals)");
        return;
    }

    const std::array<std::string, FORM_COUNT> formNames = {
        "Mario", "Super Mario", "Fiery Mario"
    };
    const std::array<std::string, FORM_COUNT> normalDirs = {
        kSmallDir, kSuperDir, kFireDir
    };
    const std::array<std::string, STAR_PALETTE_COUNT> starDirs = {
        "Star1", "Star2", "Star3"
    };

    for (std::size_t form = 0; form < FORM_COUNT; ++form) {
        const bool includeShoot = form == FormIndex(Form::FIRE);
        m_NormalVisuals[form] = CreateVisualAssets(normalDirs[form], includeShoot);
        for (std::size_t palette = 0; palette < STAR_PALETTE_COUNT; ++palette) {
            m_StarVisuals[palette][form] = CreateVisualAssets(
                MakeAssetPath("player/Effects/" + starDirs[palette] + "/" + formNames[form] + "/right/"),
                includeShoot);
        }
    }

    m_DeadImage = std::make_shared<Util::Image>(kSmallDir + "Dead/Dead.png");
    LOG_INFO("Player::InitAnimations done (normal / rotating star palettes)");
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

Player::VisualAssets Player::CreateLuigiVisualAssets(Form form) {
    VisualAssets assets;

    if (form == Form::SMALL) {
        const std::string dir = MakeAssetPath(kLuigiSmallDir);
        assets.idle = std::make_shared<Util::Image>(dir + "frame_00_16x16.png");
        assets.walk = std::make_shared<Util::Animation>(
            std::vector<std::string>{
                dir + "frame_01_16x16.png",
                dir + "frame_02_16x16.png",
                dir + "frame_03_16x16.png",
            },
            true,
            120,
            true);
        assets.jump = std::make_shared<Util::Image>(dir + "frame_05_16x16.png");
        assets.climb = std::make_shared<Util::Animation>(
            std::vector<std::string>{
                dir + "frame_10_16x16.png",
                dir + "frame_11_16x16.png",
            },
            true,
            150,
            true);
        assets.skid = std::make_shared<Util::Image>(dir + "frame_06_16x16.png");
        assets.duck = std::make_shared<Util::Image>(dir + "frame_07_16x16.png");
        return assets;
    }

    const std::string dir = MakeAssetPath(
        form == Form::FIRE ? kLuigiFireDir : kLuigiBigDir);
    const std::string idle = form == Form::FIRE ? "frame_15_16x32.png" : "frame_23_16x32.png";
    const std::array<std::string, 3> walk = form == Form::FIRE
        ? std::array<std::string, 3>{"frame_16_16x32.png", "frame_17_16x32.png", "frame_18_16x32.png"}
        : std::array<std::string, 3>{"frame_24_16x32.png", "frame_25_16x32.png", "frame_26_16x32.png"};
    const std::string jump = form == Form::FIRE ? "frame_21_16x32.png" : "frame_27_16x32.png";
    const std::string skid = form == Form::FIRE ? "frame_22_16x32.png" : "frame_28_16x32.png";
    const std::string duck = form == Form::FIRE ? "frame_33_16x32.png" : "frame_29_16x24.png";

    assets.idle = std::make_shared<Util::Image>(dir + idle);
    assets.walk = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            dir + walk[0],
            dir + walk[1],
            dir + walk[2],
        },
        true,
        120,
        true);
    assets.jump = std::make_shared<Util::Image>(dir + jump);
    assets.climb = assets.walk;
    assets.skid = std::make_shared<Util::Image>(dir + skid);
    assets.duck = std::make_shared<Util::Image>(dir + duck);
    if (form == Form::FIRE) {
        assets.shoot = std::make_shared<Util::Image>(dir + "frame_34_16x32.png");
    }
    return assets;
}

Player::Controls Player::DefaultControls() {
    return Controls{};
}

void Player::SetVisualProfile(VisualProfile profile) {
    if (m_VisualProfile == profile) return;
    m_VisualProfile = profile;
    InitAnimations();
    UpdateAnimation();
}

bool Player::AnyPressed(const std::array<Util::Keycode, 3>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](Util::Keycode key) {
        return key != Util::Keycode::UNKNOWN && Util::Input::IsKeyPressed(key);
    });
}

bool Player::AnyDown(const std::array<Util::Keycode, 3>& keys) {
    return std::any_of(keys.begin(), keys.end(), [](Util::Keycode key) {
        return key != Util::Keycode::UNKNOWN && Util::Input::IsKeyDown(key);
    });
}

bool Player::IsPressingUp() const {
    return AnyPressed(m_Controls.up);
}

bool Player::IsPressingDown() const {
    return AnyPressed(m_Controls.down);
}

bool Player::IsPressingLeft() const {
    return AnyPressed(m_Controls.left);
}

bool Player::IsPressingRight() const {
    return AnyPressed(m_Controls.right);
}

std::size_t Player::FormIndex(Form form) {
    return static_cast<std::size_t>(form);
}

const Player::VisualAssets& Player::CurrentVisualAssets() const {
    const std::size_t form = FormIndex(m_Form);

    // 受傷無敵期間沿用一般圖，外觀由可見性閃爍呈現（見 UpdateDamageInvincibility）

    if (IsStarInvincible()) {
        return m_StarVisuals[m_StarPaletteIndex][form];
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

            // 3. 三段重力：上升弱重力、頂點短暫滯空、下降強重力。
            if (!m_OnGround) {
                bool jumpHeld = AnyPressed(m_Controls.jump);
                if (!jumpHeld && m_Velocity.y < 0.0f) m_JumpCut = true;  // 放開即鎖定強重力
                float g = GRAVITY_FALL;
                if (m_Velocity.y < 0.0f && jumpHeld && !m_JumpCut) {
                    g = GRAVITY_RISE;
                } else if (m_Velocity.y >= 0.0f && m_Velocity.y <= PLAYER_APEX_HANG_SPEED) {
                    g = GRAVITY_APEX;
                }
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
                m_Velocity.y = -PLAYER_DEATH_BOUNCE;
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

                    // clearWalkTiles 表示「旗杆 X 到城堡門中心」的 tile 距離。
                    // 此刻 Mario 已在 poleX + 0.5 格，所以實際步行距離要扣掉半格。
                    m_WalkTargetX = m_Position.x +
                                    std::max(0.0f, m_LevelClearWalkTiles) * TILE_SIZE;
                    // 沒入門內的最終位置：門中心再往內 0.5 格，讓 sprite 沒進門洞。
                    m_DoorEnterX  = m_WalkTargetX + 0.5f * TILE_SIZE;

                    m_FacingLeft = false;
                    m_Velocity.x = PLAYER_MAX_WALK_SPEED;
                }
            } else if (m_IsWalkingToCastle) {
                // 自動向右走到城堡門中心
                m_Position.x += m_Velocity.x * deltaTime;
                ApplyGravity(deltaTime);

                if (m_Position.x >= m_WalkTargetX) {
                    m_Position.x = m_WalkTargetX;
                    // 還沒到門口就消失會像「半空中蒸發」；改成繼續往門內走一小段，
                    // 沒入深色城門後再隱藏（引擎無 alpha，靠門洞暗色遮掩）。
                    m_IsWalkingToCastle = false;
                    m_IsEnteringDoor = true;
                }
            } else if (m_IsEnteringDoor) {
                // 沒入城門：繼續向門內走，到門內位置才隱藏
                m_Position.x += m_Velocity.x * deltaTime;
                ApplyGravity(deltaTime);

                if (m_Position.x >= m_DoorEnterX) {
                    m_Position.x = m_DoorEnterX;
                    m_Velocity.x = 0.0f;
                    m_IsEnteringDoor = false;
                    SetVisible(false); // 進門後消失
                }
            }
            break;
        }

        case State::IntroAutoWalk: {
            UpdateDamageInvincibility(deltaTime);
            UpdateStarInvincibility(deltaTime);

            m_FacingLeft = false;
            m_IsSkidding = false;

            if (!m_IntroAutoWalkFinished) {
                m_IsMoving = true;
                m_Velocity.x = m_IntroAutoWalkSpeed;
                m_Position.x += m_Velocity.x * deltaTime;
                if (m_Position.x >= m_IntroAutoWalkTargetX) {
                    m_Position.x = m_IntroAutoWalkTargetX;
                    m_Velocity.x = 0.0f;
                    m_IsMoving = false;
                    m_IntroAutoWalkFinished = true;
                }
            }

            if (!m_OnGround) {
                ApplyGravity(deltaTime);
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
        m_DamageFlickerTimer = 0.0f;
        m_DamageFlickerVisible = true;
        SetVisible(true);
        return;
    }

    UpdateDamageFlicker(deltaTime);
}

void Player::UpdateDamageFlicker(float deltaTime) {
    m_DamageFlickerTimer += deltaTime;
    if (m_DamageFlickerTimer >= DAMAGE_FLICKER_INTERVAL) {
        m_DamageFlickerTimer = 0.0f;
        m_DamageFlickerVisible = !m_DamageFlickerVisible;
        SetVisible(m_DamageFlickerVisible);
    }
}

void Player::UpdateStarInvincibility(float deltaTime) {
    if (!IsStarInvincible()) return;

    m_StarPaletteTimer += deltaTime;
    while (m_StarPaletteTimer >= STAR_PALETTE_INTERVAL) {
        m_StarPaletteTimer -= STAR_PALETTE_INTERVAL;
        m_StarPaletteIndex = (m_StarPaletteIndex + 1) % STAR_PALETTE_COUNT;
    }

    if (m_StarInvincibleInfinite) return;

    m_StarTimer -= deltaTime;
    if (m_StarTimer <= 0.0f) {
        m_StarTimer = 0.0f;
        m_StarPaletteTimer = 0.0f;
        m_StarPaletteIndex = 0;
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
    m_TransformIsDowngrade = FormIndex(toForm) < FormIndex(fromForm);
    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
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
        // 切換 form
        m_Form = m_TransformBlinkState ? m_TransformToForm : m_TransformFromForm;
        // 同步碰撞箱尺寸到「當前顯示的形態」，並保持底部（腳）位置不變。
        // 否則大馬力歐的圖會以小箱子中心置中繪製，下半身插進腳下方塊造成破圖。
        const float oldH = m_Size.y;
        m_Size.y = (m_Form == Form::SMALL) ? TILE_SIZE : TILE_SIZE * 2.0f;
        m_Position.y += (oldH - m_Size.y);
        UpdateAnimation(); // 立即同步圖片
    }

    // 受傷縮小：大小交替同時做可見性閃爍（升級變身不閃）
    if (m_TransformIsDowngrade) {
        UpdateDamageFlicker(deltaTime);
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
        m_DamageFlickerTimer   = 0.0f;
        m_DamageFlickerVisible = true;
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

    UpdatePipeClip(camera);
}

void Player::UpdatePipeClip(const Camera& camera) {
    if (!m_ClipWrapper) return;

    if (m_State != State::EnteringPipe && m_State != State::ExitingPipe) {
        m_ClipWrapper->SetEnabled(false);
        return;
    }

    // 水管只是背景圖、無法遮住玩家，改用裁切：依開口方向，只露出管口外側的部分，
    // 讓馬力歐看起來真的「沒入 / 鑽出」水管。
    const auto  ctx   = Core::Context::GetInstance();
    const float halfW = static_cast<float>(ctx->GetWindowWidth())  * 0.5f;
    const float halfH = static_cast<float>(ctx->GetWindowHeight()) * 0.5f;

    if (m_PipeOpening == "up") {
        // 管口在水管頂面：保留頂線以上
        const float px = camera.WorldToScreen({m_Position.x, m_PipePos.y}).y + halfH;
        m_ClipWrapper->SetClipKeepAbovePx(px);
    } else if (m_PipeOpening == "down") {
        const float px = camera.WorldToScreen({m_Position.x, m_PipePos.y + m_PipeSize.y}).y + halfH;
        m_ClipWrapper->SetClipKeepBelowPx(px);
    } else if (m_PipeOpening == "left") {
        // 管口在水管左面：保留左側
        const float px = camera.WorldToScreen({m_PipePos.x, m_Position.y}).x + halfW;
        m_ClipWrapper->SetClipKeepLeftPx(px);
    } else if (m_PipeOpening == "right") {
        const float px = camera.WorldToScreen({m_PipePos.x + m_PipeSize.x, m_Position.y}).x + halfW;
        m_ClipWrapper->SetClipKeepRightPx(px);
    }
    m_ClipWrapper->SetEnabled(true);
}

// ─── 動畫切換邏輯 ────────────────────────────────────────────────────

void Player::UpdateAnimation() {
    // 小馬力歐 Y 身體只有一格，SUPER/FIRE 是兩格
    // SetForm() 已調整 m_Size，這裡只要切换圖片

    const VisualAssets& visuals = CurrentVisualAssets();

    if (m_State == State::EnteringPipe || m_State == State::ExitingPipe) {
        if (m_PipeOpening == "up" || m_PipeOpening == "down") {
            // 垂直鑽管時不換成蹲下圖，沿用當前形態的待機（站立）圖
            SetVisual(visuals.idle);
        } else {
            SetVisual(visuals.walk);
        }
        return;
    }

    if (m_State == State::IntroAutoWalk) {
        SetVisual(visuals.walk);
        return;
    }

    if (m_State == State::Dying) {
        SetVisual(m_DeadImage);
        return;
    }

    // 變身動畫中：顯示當前 m_Form 的待機圖（m_Form 由閃爍邏輯交替切換）
    if (m_State == State::Transforming) {
        SetVisual(visuals.idle);
        return;
    }

    if (m_State == State::LevelClear) {
        if (m_IsSlidingDown) {
            SetVisual(visuals.climb);
        } else if (m_IsWalkingToCastle || m_IsEnteringDoor) {
            SetVisual(visuals.walk);
        } else {
            SetVisual(visuals.idle);
        }
        return;
    }

    if (m_Form == Form::FIRE && m_ShootingTimer > 0.0f) {
        SetVisual(visuals.shoot);
        return;
    }

    if (!m_OnGround) {
        SetVisual(visuals.jump);
    } else if (m_IsSkidding) {
        SetVisual(visuals.skid);
    } else if (m_IsMoving) {
        SetVisual(visuals.walk);
    } else {
        SetVisual(visuals.idle);
    }
}

// ─── 鍵盤輸入 ────────────────────────────────────────────────────────

void Player::HandleInput(float deltaTime) {
    if (AnyDown(m_Controls.debugSmall)) {
        SetForm(Form::SMALL);
    } else if (AnyDown(m_Controls.debugSuper)) {
        SetForm(Form::SUPER);
    } else if (AnyDown(m_Controls.debugFire)) {
        SetForm(Form::FIRE);
    }
    if (AnyDown(m_Controls.debugStar)) {
        ToggleDebugStarInvincibility();
    }

    bool pressingLeft = AnyPressed(m_Controls.left);
    bool pressingRight = AnyPressed(m_Controls.right);
    bool holdingRun = AnyPressed(m_Controls.run);

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

    // 是否在「移動」供動畫播放判斷：
    //  - 有按單一方向鍵即視為移動意圖：即使貼牆被擋下、vx 被碰撞歸零，
    //    走路動畫仍持續（符合「走路動畫照方向鍵跑」）。
    //  - 或仍有殘餘速度：放開方向鍵後的滑行階段也維持走路動畫。
    const bool directionalIntent = (pressingLeft != pressingRight);
    m_IsMoving = directionalIntent || std::abs(m_Velocity.x) > 5.0f;

    // 跳躍（只有站在地上才能跳）
    const bool jumpDown = AnyDown(m_Controls.jump);
    if (m_OnGround && jumpDown) {
        Jump();
    }

    // 射擊火球（只有 FIRE 形態可以，預設 Z 鍵）
    if (m_Form == Form::FIRE && AnyDown(m_Controls.fire)) {
        m_ShootRequested = true;
    }
}

void Player::Jump() {
    float launch = PLAYER_JUMP_VELOCITY;
    if (std::abs(m_Velocity.x) > PLAYER_RUN_JUMP_THRESHOLD)          // |vx| > 90
        launch += PLAYER_JUMP_RUN_BONUS;
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
    m_IsEnteringDoor = false;
    m_WalkTargetX = 0.0f;
    m_DoorEnterX = 0.0f;
    m_LevelClearWalkTiles = DEFAULT_LEVEL_CLEAR_WALK_TILES;
    m_IntroAutoWalkTargetX = 0.0f;
    m_IntroAutoWalkSpeed = 0.0f;
    m_IntroAutoWalkFinished = false;
    m_DamageInvincibleTimer = 0.0f;
    m_StarTimer = 0.0f;
    m_StarInvincibleInfinite = false;
    m_StarPaletteTimer = 0.0f;
    m_StarPaletteIndex = 0;
    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
    // 變身動畫狀態重置
    m_TransformTimer     = 0.0f;
    m_TransformBlinkTimer = 0.0f;
    m_TransformBlinkState = false;
    m_TransformIsDowngrade = false;
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

void Player::StartLevelClearSequence(float poleX, float bottomY, float clearWalkTiles) {
    m_State = State::LevelClear;
    m_IsLevelCleared = true;
    m_IsSlidingDown = true;
    m_IsWalkingToCastle = false;
    m_IsEnteringDoor = false;
    m_DamageInvincibleTimer = 0.0f;
    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
    SetVisible(true);

    m_Position.x = poleX + TILE_SIZE * 0.5f;

    m_PoleBottomY = bottomY;
    m_LevelClearWalkTiles = std::max(0.0f, clearWalkTiles);

    // 與旗子同速下滑（FlagBlock::DESCENT_SPEED 也用 POLE_SLIDE_SPEED）
    m_Velocity = {0.0f, POLE_SLIDE_SPEED};
    
    m_FacingLeft = true;

    LOG_INFO("Player level clear sequence started. Target bottomY={} clearWalkTiles={}",
             bottomY,
             m_LevelClearWalkTiles);
}

void Player::StartIntroAutoWalk(float targetX, float walkSpeed) {
    m_State = State::IntroAutoWalk;
    m_IntroAutoWalkTargetX = targetX;
    m_IntroAutoWalkSpeed = std::max(0.0f, walkSpeed);
    m_IntroAutoWalkFinished = false;
    m_FacingLeft = false;
    m_IsSkidding = false;
    m_IsMoving = true;
    m_Velocity.x = m_IntroAutoWalkSpeed;
    SetVisible(true);
    LOG_INFO("Player intro auto-walk started: targetX={} speed={}", targetX, m_IntroAutoWalkSpeed);
}

void Player::StartDamageInvincibility(float duration) {
    if (duration <= 0.0f || m_State == State::Dying || m_State == State::LevelClear) return;

    m_DamageInvincibleTimer = duration;
    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
    SetVisible(true);
}

void Player::ActivateStarInvincibility(float duration) {
    if (duration <= 0.0f || m_State == State::Dying || m_State == State::LevelClear) return;
    m_StarInvincibleInfinite = false;
    m_StarTimer = duration;
    m_StarPaletteTimer = 0.0f;
    m_StarPaletteIndex = 0;
    SetVisible(true);
    LOG_INFO("Player star invincibility activated for {} seconds.", duration);
}

void Player::ToggleDebugStarInvincibility() {
    if (m_State == State::Dying || m_State == State::LevelClear) return;

    if (m_StarInvincibleInfinite || m_StarTimer > 0.0f) {
        const bool timedStarWasActive = m_StarTimer > 0.0f;
        m_StarInvincibleInfinite = false;
        m_StarTimer = 0.0f;
        m_StarPaletteTimer = 0.0f;
        m_StarPaletteIndex = 0;
        if (timedStarWasActive) {
            m_StarEndedEventPending = true;
        }
        UpdateAnimation();
        LOG_INFO("Debug star invincibility disabled.");
        return;
    }

    m_StarInvincibleInfinite = true;
    m_StarPaletteTimer = 0.0f;
    m_StarPaletteIndex = 0;
    SetVisible(true);
    UpdateAnimation();
    LOG_INFO("Debug star invincibility enabled.");
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
            // SMB1 規則：火焰瑪利歐受傷直接縮回小瑪利歐。
            StartTransformAnimation(Form::FIRE, Form::SMALL);
            LOG_INFO("Player downgraded: FIRE -> SMALL (transform animation)");
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
            m_StarInvincibleInfinite = false;
            m_StarPaletteTimer = 0.0f;
            m_StarPaletteIndex = 0;
            m_Velocity = {0.0f, 0.0f};
            SetVisual(m_DeadImage);
            SetVisible(true);
            LOG_INFO("Player died! (SMALL hit by enemy) - Starting death animation");
            break;
    }
}

// ─── 水管動畫專屬實作 ───────────────────────────────────────────────────

void Player::StartPipeEntry(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration) {
    m_State = State::EnteringPipe;
    m_PipeOpening = opening;
    m_PipePos  = pipePosition;
    m_PipeSize = pipeSize;
    m_AnimTimer = 0.0f;
    m_AnimDuration = duration;
    m_Velocity = {0.0f, 0.0f};

    // 水管動畫期間無敵計時暫停，閃爍若停在隱形幀會整段消失，強制恢復可見
    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
    SetVisible(true);

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
        float targetY = pipePosition.y + pipeSize.y - m_Size.y;
        m_AnimStartPos = {pipePosition.x - m_Size.x, targetY};
        m_AnimEndPos = {pipePosition.x, targetY};
    } else if (opening == "right") {
        float targetY = pipePosition.y + pipeSize.y - m_Size.y;
        m_AnimStartPos = {pipePosition.x + pipeSize.x, targetY};
        m_AnimEndPos = {pipePosition.x + pipeSize.x - m_Size.x, targetY};
    }

    m_Position = m_AnimStartPos;
    LOG_INFO("Player pipe entry started: opening={} startPos={} endPos={}", opening, m_AnimStartPos, m_AnimEndPos);
}

void Player::StartPipeExit(glm::vec2 pipePosition, glm::vec2 pipeSize, const std::string& opening, float duration) {
    m_State = State::ExitingPipe;
    m_PipeOpening = opening;
    m_PipePos  = pipePosition;
    m_PipeSize = pipeSize;
    m_AnimTimer = 0.0f;
    m_AnimDuration = duration;
    m_Velocity = {0.0f, 0.0f};

    m_DamageFlickerTimer   = 0.0f;
    m_DamageFlickerVisible = true;
    SetVisible(true);

    if (opening == "up") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y};
        m_AnimEndPos = {targetX, pipePosition.y - m_Size.y};
    } else if (opening == "down") {
        float targetX = pipePosition.x + pipeSize.x * 0.5f - m_Size.x * 0.5f;
        m_AnimStartPos = {targetX, pipePosition.y + pipeSize.y - m_Size.y};
        m_AnimEndPos = {targetX, pipePosition.y + pipeSize.y};
    } else if (opening == "left") {
        float targetY = pipePosition.y + pipeSize.y - m_Size.y;
        m_AnimStartPos = {pipePosition.x, targetY};
        m_AnimEndPos = {pipePosition.x - m_Size.x, targetY};
    } else if (opening == "right") {
        float targetY = pipePosition.y + pipeSize.y - m_Size.y;
        m_AnimStartPos = {pipePosition.x + pipeSize.x - m_Size.x, targetY};
        m_AnimEndPos = {pipePosition.x + pipeSize.x, targetY};
    }

    m_Position = m_AnimStartPos;
    LOG_INFO("Player pipe exit started: opening={} startPos={} endPos={}", opening, m_AnimStartPos, m_AnimEndPos);
}
