#include "GameManager.hpp"

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "LevelLoader.hpp"
#include "CollisionUtils.hpp"
#include "Core/Context.hpp"
#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
#include "Util/Text.hpp"
#include "Util/Time.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

#include "MushroomItem.hpp"
#include "FireFlowerItem.hpp"
#include "CoinItem.hpp"
#include "HiddenBlock.hpp"
#include "Koopa.hpp"
#include "KoopaParatroopa.hpp"
#include "LevelCoinItem.hpp"
#include "MultiCoinBlock.hpp"
#include "OneUpMushroomItem.hpp"
#include "StarmanItem.hpp"

// ─── 關卡 JSON 路徑 ────────────────────────────────────────────────────────
// RESOURCE_DIR 定義在 CMakeLists.txt，指向 Mario/Resources/
// data/ 資料夾與 Resources/ 同層，所以往上一層再進 data/
static const std::string kLevel1_1 =
    std::string(RESOURCE_DIR) + "/../data/1-1.json";

// ─── Start ────────────────────────────────────────────────────────────────

void GameManager::Start() {
    EnterTitleScreen();
}

// ─── Update ───────────────────────────────────────────────────────────────

void GameManager::Update() {
    const float rawDt = Util::Time::GetDeltaTimeMs() / 1000.0f;
    const float dt = std::min(rawDt, MAX_FRAME_DT);

    if (rawDt > MAX_FRAME_DT) {
        LOG_WARN("Large frame dt detected: {}s, clamped to {}s", rawDt, dt);
    }

    switch (m_FlowState) {
        case FlowState::Title:
            UpdateTitle(dt);
            break;
        case FlowState::LevelIntro:
            UpdateLevelIntro(dt);
            break;
        case FlowState::Playing:
            UpdatePlaying(dt);
            break;
        case FlowState::TimeUp:
            UpdateTimeUp(dt);
            break;
        case FlowState::LevelClearTransition:
            UpdateLevelClearTransition(dt);
            break;
    }
}

void GameManager::UpdateTitle(float dt) {
    (void)dt;

    if (Util::Input::IsKeyDown(Util::Keycode::RETURN) ||
        Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        StartNewGame();
        return;
    }

    DrawScene(false);
}

void GameManager::UpdateLevelIntro(float dt) {
    m_StateTimer += dt;
    if (m_StateTimer >= 2.0f) {
        EnterPlaying();
        return;
    }

    m_Renderer.Update();
}

void GameManager::UpdatePlaying(float dt) {
    // 1. 更新所有角色邏輯
    m_Player.Update(dt);

    // 如果瑪利歐正在播放死亡動畫，停止更新世界其他物件（凍結畫面）
    if (!m_Player.IsDying()) {
        for (auto& enemy : m_Enemies) enemy->Update(dt);
        for (auto& block : m_Blocks)  block->Update(dt);
        for (auto& item : m_Items)    item->Update(dt);
        for (auto& fireball : m_Fireballs) fireball->Update(dt);
        for (auto& debris : m_BrickDebris) debris->Update(dt);

        m_BrickDebris.erase(
            std::remove_if(m_BrickDebris.begin(), m_BrickDebris.end(),
                           [](const auto& debris) { return debris->IsDead(); }),
            m_BrickDebris.end());

        if (m_Player.ConsumeShootRequest()) {
            // 限制畫面上最多只能同時有 2 顆火球
            if (m_Fireballs.size() < 2) {
                // 火球生成在瑪利歐前方一點點
                glm::vec2 spawnPos = m_Player.GetPosition();
                spawnPos.x += m_Player.IsFacingLeft() ? -8.0f : 8.0f;
                SpawnFireball(spawnPos, m_Player.IsFacingLeft());
            }
        }

        // 2. 更新鏡頭（跟隨玩家，夾制在地圖邊界內）
        m_Camera.Update(m_Player.GetPosition().x,
                        static_cast<float>(m_Level.levelWidth));

        // 2.5 鏡頭觸發：檢查是否有新敵人進入鏡頭範圍
        CheckEnemySpawnQueue();

        // 3. 執行碰撞偵測 (包含方塊與敵人)
        CheckBlockCollision();
        if (CheckPipeTransition()) {
            m_Camera.Update(m_Player.GetPosition().x,
                            static_cast<float>(m_Level.levelWidth));
        } else {
            CheckEnemyBlockCollision();
            CheckItemBlockCollision();
            CheckItemCollision();
            CheckStompCollision();
            CheckFireballCollision();
            CheckFlagCollision();
        }
    }

    // ─── 虛空掉落判定 (Kill Z) ───
    const float killZ = m_Level.levelHeight + 50.0f;
    if (m_Player.IsAlive() && m_Player.GetPosition().y > killZ) {
        m_Player.SetAlive(false);
        LOG_INFO("Player fell into the void! Life lost.");
        HandleLifeLost();
        return;
    }

    // 順便把掉出畫面的敵人也清除
    for (auto& enemy : m_Enemies) {
        if (enemy->IsAlive() && enemy->GetPosition().y > killZ) {
            enemy->SetAlive(false);
            enemy->SetVisible(false);
        }
    }

    if (m_LevelCleared && m_Player.IsLevelClearSequenceFinished()) {
        EnterLevelClearTransition();
        UpdateLevelClearTransition(0.0f);
        return;
    }

    if (!m_LevelCleared && m_Player.IsAlive()) {
        m_TimeRemaining -= dt;
        if (m_TimeRemaining <= 0.0f) {
            m_TimeRemaining = 0.0f;
            EnterTimeUp();
            return;
        }
    }

    DrawScene(true);
}

void GameManager::UpdateTimeUp(float dt) {
    m_StateTimer += dt;
    if (m_StateTimer >= 2.0f) {
        HandleLifeLost();
        return;
    }

    m_Renderer.Update();
}

void GameManager::UpdateLevelClearTransition(float dt) {
    m_LevelClearTransitionTimer += dt;
    if (m_LevelClearTransitionTimer >= 3.0f) {
        EnterTitleScreen();
        DrawScene(false);
        return;
    }

    m_Renderer.Update();
}

void GameManager::DrawScene(bool updateHud) {
    // 4. 同步畫面位置（把世界座標換算成螢幕座標）
    m_Player.Draw(m_Camera);
    for (auto& enemy : m_Enemies) enemy->Draw(m_Camera);
    for (auto& block : m_Blocks)  block->Draw(m_Camera);
    for (auto& item : m_Items)    item->Draw(m_Camera);
    for (auto& fireball : m_Fireballs) fireball->Draw(m_Camera);
    for (auto& debris : m_BrickDebris) debris->Draw(m_Camera);

    // 5. 處理背景圖座標換算與縮放
    if (m_Background) {
        float halfW = m_Level.levelWidth / 2.0f;
        float halfH = m_Level.levelHeight / 2.0f;
        m_Background->m_Transform.translation = m_Camera.WorldToScreen({halfW, halfH});
        m_Background->m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    }

    // 6. 更新 HUD（倒數計時器 + 文字內容）
    if (updateHud) {
        const auto& progress = m_Session.CurrentPlayer();
        m_Hud.Update(progress.score, progress.coins, static_cast<int>(m_TimeRemaining));
    }

    // 7. 渲染到畫面上
    m_Renderer.Update();
}

void GameManager::ResetSceneObjects() {
    m_Enemies.clear();
    m_EnemySpawnQueue.clear();
    m_Items.clear();
    m_Blocks.clear();
    m_Fireballs.clear();
    m_BrickDebris.clear();
    m_OverlayObjects.clear();
    m_Background.reset();
    m_Renderer = Util::Renderer();
}

void GameManager::StartNewGame() {
    ResetSceneObjects();
    m_Session.ResetNewGame(1);
    m_Player.ResetForNewGame();

    EnterLevelIntro();
    LOG_INFO("New game started.");
}

void GameManager::EnterLevelIntro() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 400.0f;
    m_LevelCleared = false;

    m_Hud.Init(m_Renderer);
    const auto& progress = m_Session.CurrentPlayer();
    m_Hud.Update(progress.score, progress.coins, static_cast<int>(m_TimeRemaining));
    BuildLevelIntroOverlay();

    m_FlowState = FlowState::LevelIntro;
    LOG_INFO("Entered level intro: player='{}' lives={}",
             m_Session.GetCurrentPlayerName(),
             progress.lives);
}

void GameManager::EnterPlaying() {
    ResetSceneObjects();
    m_TimeRemaining = 400.0f;
    m_LevelCleared = false;

    LoadLevel(kLevel1_1);
    ApplyPlayerProgress();
    m_Camera.Update(m_Player.GetPosition().x, static_cast<float>(m_Level.levelWidth));
    BuildScene();
    m_Hud.Init(m_Renderer);

    m_FlowState = FlowState::Playing;
    LOG_INFO("Entered playing state.");
}

void GameManager::EnterTitleScreen() {
    ResetSceneObjects();
    m_LevelCleared = false;
    m_StateTimer = 0.0f;
    m_LevelClearTransitionTimer = 0.0f;
    m_Player.ResetForNewGame();

    LoadLevel(kLevel1_1);
    m_Camera.Update(m_Player.GetPosition().x, static_cast<float>(m_Level.levelWidth));
    BuildScene();
    BuildTitleOverlay();

    m_FlowState = FlowState::Title;
    LOG_INFO("Entered title screen.");
}

void GameManager::EnterTimeUp() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 0.0f;
    m_LevelCleared = false;

    m_Hud.Init(m_Renderer);
    const auto& progress = m_Session.CurrentPlayer();
    m_Hud.Update(progress.score, progress.coins, 0);
    BuildTimeUpOverlay();

    m_FlowState = FlowState::TimeUp;
    LOG_INFO("Entered time up state.");
}

void GameManager::EnterLevelClearTransition() {
    SavePlayerProgress();
    ResetSceneObjects();
    m_LevelClearTransitionTimer = 0.0f;
    BuildLevelClearOverlay();
    m_FlowState = FlowState::LevelClearTransition;
    LOG_INFO("Entered level clear transition.");
}

void GameManager::BuildTitleOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    AddOverlayImage("ui/title/logo.png", {0.0f, halfH - 180.0f}, {2.5f, 2.5f});
    AddOverlayText("PRESS ENTER OR SPACE", 16, {0.0f, halfH - 310.0f});
}

void GameManager::BuildLevelIntroOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    AddOverlayText("WORLD  1-1", 22, {0.0f, halfH - 260.0f});

    std::ostringstream livesText;
    livesText << m_Session.GetCurrentPlayerName()
              << " x " << m_Session.CurrentPlayer().lives;
    AddOverlayText(livesText.str(), 18, {0.0f, halfH - 380.0f});
}

void GameManager::BuildTimeUpOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    AddOverlayText("TIME UP", 24, {0.0f, halfH - 430.0f});
}

void GameManager::BuildLevelClearOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    std::ostringstream scoreText;
    scoreText << "SCORE " << std::setw(6) << std::setfill('0')
              << m_Session.CurrentPlayer().score;

    AddOverlayText("WORLD CLEAR", 24, {0.0f, halfH - 190.0f});
    AddOverlayText(scoreText.str(), 16, {0.0f, halfH - 260.0f});
    AddOverlayText("RETURNING TO TITLE", 14, {0.0f, halfH - 320.0f});
}

void GameManager::AddOverlayText(const std::string& text, int fontSize, glm::vec2 position, float zIndex) {
    const std::string fontPath = MakeAssetPath("font/Super Mario Bros. NES.ttf");
    const auto drawable = std::make_shared<Util::Text>(
        fontPath,
        fontSize,
        text,
        Util::Color(255, 255, 255));

    auto obj = std::make_shared<Util::GameObject>(drawable, zIndex);
    obj->m_Transform.translation = position;
    obj->SetVisible(true);

    m_OverlayObjects.push_back(obj);
    m_Renderer.AddChild(obj);
}

void GameManager::AddOverlayImage(const std::string& assetPath, glm::vec2 position, glm::vec2 scale, float zIndex) {
    auto drawable = std::make_shared<Util::Image>(MakeAssetPath(assetPath));

    auto obj = std::make_shared<Util::GameObject>(drawable, zIndex);
    obj->m_Transform.translation = position;
    obj->m_Transform.scale = scale;
    obj->SetVisible(true);

    m_OverlayObjects.push_back(obj);
    m_Renderer.AddChild(obj);
}

void GameManager::SavePlayerProgress() {
    auto& progress = m_Session.CurrentPlayer();
    progress.form = m_Player.GetForm();
}

void GameManager::ApplyPlayerProgress() {
    m_Player.SetForm(m_Session.CurrentPlayer().form);
}

void GameManager::HandleLifeLost() {
    SavePlayerProgress();
    m_Session.LoseLife();

    if (m_Session.IsGameOver()) {
        LOG_INFO("Game over. Returning to title.");
        EnterTitleScreen();
        return;
    }

    m_Session.SwitchToNextAlivePlayer();
    EnterLevelIntro();
}

// ─── LoadLevel ────────────────────────────────────────────────────────────

void GameManager::LoadLevel(const std::string& jsonPath) {
    m_Level = LevelLoader::Load(jsonPath);

    LOG_INFO(
        "Level loaded: json='{}' background='{}' levelSize=({}, {}) playerSpawn={}",
        jsonPath,
        m_Level.backgroundImagePath,
        m_Level.levelWidth,
        m_Level.levelHeight,
        m_Level.playerSpawn
    );

    // ── 背景圖 ──────────────────────────────────────────────────────
    if (!m_Level.backgroundImagePath.empty()) {
        auto image = std::make_shared<Util::Image>(
            MakeAssetPath(m_Level.backgroundImagePath));
        m_Background = std::make_shared<Util::GameObject>(image, Z_INDEX_BACKGROUND);
        m_Background->SetVisible(true);
        LOG_INFO(
            "Background sprite path: {} size={}",
            MakeAssetPath(m_Level.backgroundImagePath),
            image->GetSize()
        );
    }

    // ── 方塊 & 物件 ────────────────────────────────────────────────
    for (const auto& obj : m_Level.objects) {
        const glm::vec2 pos = {obj.x, obj.y};
        const glm::vec2 size = {obj.width, obj.height};

        if (obj.type == "Ground") {
            m_Blocks.push_back(std::make_shared<GroundBlock>(pos, size));
        } else if (obj.type == "Brick") {
            auto b = std::make_shared<BrickBlock>(pos, m_Level.theme);
            b->SetItemType(obj.itemType.empty() ? "None" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "QuestionBlock") {
            auto b = std::make_shared<QuestionBlock>(pos, m_Level.theme);
            b->SetItemType(obj.itemType.empty() ? "Coin" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "HiddenBlock") {
            auto b = std::make_shared<HiddenBlock>(pos, m_Level.theme);
            b->SetItemType(obj.itemType.empty() ? "Coin" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "MultiCoinBlock") {
            m_Blocks.push_back(std::make_shared<MultiCoinBlock>(pos, m_Level.theme, obj.coinCount));
        } else if (obj.type == "Pipe") {
            m_Blocks.push_back(std::make_shared<PipeBlock>(
                pos,
                size,
                obj.opening,
                obj.enterable,
                obj.targetLevel,
                obj.exitToLevel,
                obj.targetSpawn,
                obj.hasTargetSpawn,
                m_Level.theme));
        } else if (obj.type == "MovingPlatform") {
            glm::vec2 platformSize = size;
            if (platformSize.x <= 0.0f) platformSize.x = TILE_SIZE * 3.0f;
            if (platformSize.y <= 0.0f) platformSize.y = TILE_SIZE * 0.5f;
            m_Blocks.push_back(std::make_shared<MovingPlatformBlock>(
                pos,
                platformSize,
                obj.moveAxis,
                obj.moveDistance,
                obj.moveSpeed,
                obj.moveMode));
        } else if (obj.type == "TreePlatform") {
            m_Blocks.push_back(std::make_shared<TreePlatformBlock>(pos, obj.segments));
        } else if (obj.type == "Wall") {
            m_Blocks.push_back(std::make_shared<WallBlock>(pos, m_Level.theme));
        } else if (obj.type == "Flag") {
            m_Blocks.push_back(std::make_shared<FlagBlock>(pos));
        } else if (obj.type == "Coin" || obj.type == "CollectibleCoin") {
            m_Items.push_back(std::make_shared<LevelCoinItem>(pos));
        } else if (obj.type == "EnemySpawn") {
            // 不直接建立物件，改存進 queue，等鏡頭到達再生成
            m_EnemySpawnQueue.push_back(obj);
            LOG_INFO("EnemySpawn queued: type='{}' at ({}, {})", obj.enemyType, obj.x, obj.y);
        }
    }

    // 將瑪利歐放在獨立的出生點
    m_Player.SetSpawnPosition(m_Level.playerSpawn);
    LOG_INFO("Player spawn applied: {}", m_Player.GetPosition());
}

std::string GameManager::MakeLevelPath(const std::string& levelName) {
    if (levelName.empty()) return "";

    const bool hasJsonSuffix =
        levelName.size() >= 5 &&
        levelName.substr(levelName.size() - 5) == ".json";

    return std::string(RESOURCE_DIR) + "/../data/" +
           levelName + (hasJsonSuffix ? "" : ".json");
}

void GameManager::ChangeLevel(const std::string& levelName, std::optional<glm::vec2> spawnOverride) {
    const std::string levelPath = MakeLevelPath(levelName);
    if (levelPath.empty()) return;

    LOG_INFO("Changing level to '{}'", levelPath);

    ResetSceneObjects();
    m_LevelCleared = false;

    LoadLevel(levelPath);
    if (spawnOverride.has_value()) {
        m_Player.SetSpawnPosition(*spawnOverride);
        LOG_INFO("Player spawn override applied: {}", m_Player.GetPosition());
    }

    BuildScene();
    m_Hud.Init(m_Renderer);
}

bool GameManager::CheckPipeTransition() {
    const bool pressingDown =
        Util::Input::IsKeyDown(Util::Keycode::DOWN) ||
        Util::Input::IsKeyDown(Util::Keycode::S);
    const bool pressingUp =
        Util::Input::IsKeyDown(Util::Keycode::UP) ||
        Util::Input::IsKeyDown(Util::Keycode::W);
    const bool pressingLeft =
        Util::Input::IsKeyDown(Util::Keycode::LEFT) ||
        Util::Input::IsKeyDown(Util::Keycode::A);
    const bool pressingRight =
        Util::Input::IsKeyDown(Util::Keycode::RIGHT) ||
        Util::Input::IsKeyDown(Util::Keycode::D);

    const glm::vec2 pPos = m_Player.GetPosition();
    const glm::vec2 pSize = m_Player.GetSize();
    const float playerCenterX = pPos.x + pSize.x * 0.5f;
    const float playerCenterY = pPos.y + pSize.y * 0.5f;
    const float playerBottom = pPos.y + pSize.y;

    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;

        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe || !pipe->IsEnterable()) continue;

        const glm::vec2 pipePos = pipe->GetPosition();
        const glm::vec2 pipeSize = pipe->GetSize();
        const std::string opening = pipe->GetOpening();

        bool canEnter = false;
        if (opening == "up") {
            const bool horizontallyAligned =
                playerCenterX >= pipePos.x &&
                playerCenterX <= pipePos.x + pipeSize.x;
            const bool standingOnMouth =
                std::abs(playerBottom - pipePos.y) <= 8.0f;
            canEnter = pressingDown && horizontallyAligned && standingOnMouth;
        } else if (opening == "down") {
            const bool horizontallyAligned =
                playerCenterX >= pipePos.x &&
                playerCenterX <= pipePos.x + pipeSize.x;
            const bool touchingMouth =
                std::abs(pPos.y - (pipePos.y + pipeSize.y)) <= 8.0f;
            canEnter = pressingUp && horizontallyAligned && touchingMouth;
        } else if (opening == "left") {
            const bool verticallyAligned =
                playerCenterY >= pipePos.y &&
                playerCenterY <= pipePos.y + pipeSize.y;
            const bool touchingMouth =
                std::abs((pPos.x + pSize.x) - pipePos.x) <= 8.0f;
            canEnter = pressingRight && verticallyAligned && touchingMouth;
        } else if (opening == "right") {
            const bool verticallyAligned =
                playerCenterY >= pipePos.y &&
                playerCenterY <= pipePos.y + pipeSize.y;
            const bool touchingMouth =
                std::abs(pPos.x - (pipePos.x + pipeSize.x)) <= 8.0f;
            canEnter = pressingLeft && verticallyAligned && touchingMouth;
        }

        if (!canEnter) continue;

        std::string targetLevel = pipe->GetTargetLevel();
        if (targetLevel.empty()) targetLevel = pipe->GetExitToLevel();
        if (targetLevel.empty()) {
            LOG_WARN("Enterable pipe at {} has no target level.", pipePos);
            return false;
        }

        std::optional<glm::vec2> spawnOverride;
        if (pipe->HasTargetSpawn()) {
            spawnOverride = pipe->GetTargetSpawn();
        }

        ChangeLevel(targetLevel, spawnOverride);
        return true;
    }

    return false;
}

// ─── CheckEnemySpawnQueue & SpawnEnemy ──────────────────────────────────────

/*
 * 每幀掃描待生成清單，判斷敵人的世界 X 是否進入鏡頭右邊界。
 * 为了讓玩家看到料敌人，加一小段緩衝距離（SPAWN_BUFFER）再生成。
 * 生成後從 queue 移除。
 */
void GameManager::CheckEnemySpawnQueue() {
    // 右邊界 = 鏡頭左邊界 + 可視世界寬度 + 一小段預載
    constexpr float SPAWN_BUFFER = 32.0f; // 默默在画面外儴 2 格再生成
    const float cameraRight = m_Camera.GetX() + m_Camera.GetViewWorldWidth() + SPAWN_BUFFER;

    std::vector<ObjectData> remaining;
    remaining.reserve(m_EnemySpawnQueue.size());

    for (const auto& data : m_EnemySpawnQueue) {
        if (data.x <= cameraRight) {
            // 進入觀察範圍，真的建立敷人物件
            SpawnEnemy(data);
        } else {
            // 還沒到，保留在 queue 裡
            remaining.push_back(data);
        }
    }

    m_EnemySpawnQueue = std::move(remaining);
}

/*
 * 真的建立敵人物件並加入渲染器
 */
void GameManager::SpawnEnemy(const ObjectData& data) {
    std::shared_ptr<Enemy> newEnemy;

    const auto koopaVariant =
        (data.variant == "red") ? Koopa::Variant::Red : Koopa::Variant::Green;
    const auto flightMode =
        (data.flightMode == "verticalPatrol") ? KoopaParatroopa::FlightMode::VerticalPatrol
                                              : KoopaParatroopa::FlightMode::Hop;

    if      (data.enemyType == "Goomba")       newEnemy = std::make_shared<Goomba>(data.x, data.y);
    else if (data.enemyType == "Koopa")        newEnemy = std::make_shared<Koopa>(data.x, data.y, koopaVariant);
    else if (data.enemyType == "KoopaParatroopa") {
        newEnemy = std::make_shared<KoopaParatroopa>(data.x, data.y, koopaVariant, flightMode);
    }
    else if (data.enemyType == "PiranhaPlant") newEnemy = std::make_shared<PiranhaPlant>(data.x, data.y);

    if (newEnemy) {
        m_Enemies.push_back(newEnemy);
        m_Renderer.AddChild(newEnemy);
        LOG_INFO("Enemy spawned by camera trigger: type='{}' at ({}, {})",
                 data.enemyType, data.x, data.y);
    }
}

// ─── CheckStompCollision ──────────────────────────────────────────────────
void GameManager::CheckStompCollision() {
    if (!m_Player.IsAlive()) return;

    const glm::vec2 pPos  = m_Player.GetPosition();
    const glm::vec2 pSize = m_Player.GetSize();
    const glm::vec2 pPrev = m_Player.GetPreviousPosition();
    const glm::vec2 pVel  = m_Player.GetVelocity();

    for (auto& enemy : m_Enemies) {
        if (!enemy->IsAlive()) continue;

        const glm::vec2 ePos  = enemy->GetPosition();
        const glm::vec2 eSize = enemy->GetSize();

        // 矩形重疊判斷（AABB）
        if (!CollisionUtils::CheckAABB(pPos, pSize, ePos, eSize)) continue;

        if (m_Player.IsStarInvincible()) {
            enemy->SetAlive(false);
            enemy->SetVisible(false);
            m_Session.AddScore(200);
            LOG_INFO("Star invincibility defeated enemy.");
            continue;
        }

        // ── 判斷是否為靜止的龜殼 ──
        auto* koopa = dynamic_cast<Koopa*>(enemy.get());
        bool isStationaryShell = koopa && koopa->IsInShell() && !koopa->IsSliding();
        const bool isPiranha = dynamic_cast<PiranhaPlant*>(enemy.get()) != nullptr;

        // ── 踩踏判定：必須是玩家從上方往下落到敵人頭上 ──
        const float previousBottom = pPrev.y + pSize.y;
        const float playerBottom = pPos.y + pSize.y;
        const float enemyTop     = ePos.y;
        const bool fallingOntoEnemy =
            pVel.y > 0.0f &&
            previousBottom <= enemyTop + 6.0f &&
            playerBottom <= enemyTop + 12.0f;

        if (fallingOntoEnemy && !isPiranha) {
            // TODO研究程式碼
            if (isStationaryShell) {
                // 從上方踩到縮殼中的龜 → 踢殼！
                const bool playerIsLeftOfShell =
                    (pPos.x + pSize.x * 0.5f) < (ePos.x + eSize.x * 0.5f);
                const bool kickLeft = !playerIsLeftOfShell;
                koopa->Kick(kickLeft);
                LOG_INFO("Koopa shell kicked {} from top!", kickLeft ? "left" : "right");
            } else {
                // 一般踩踏（包含首次踩到正常行走的龜）
                enemy->Stomp();
            }

            // 踩踏後給玩家一個小彈跳
            glm::vec2 vel = m_Player.GetVelocity();
            vel.y = -180.0f;
            m_Player.SetVelocity(vel);
        } else {
            // ── 側碰 / 從下方衝入 ──
            if (isStationaryShell) {
                // 從側面碰到靜止的龜殼 → 直接踢飛，且不會有往上的彈跳！
                const bool playerIsLeftOfShell =
                    (pPos.x + pSize.x * 0.5f) < (ePos.x + eSize.x * 0.5f);
                const bool kickLeft = !playerIsLeftOfShell;
                koopa->Kick(kickLeft);
                LOG_INFO("Koopa shell kicked {} from side!", kickLeft ? "left" : "right");
            } else {
                // 碰到有殺傷力的敵人：玩家受傷降級
                if (!m_Player.IsDamageInvincible()) {
                    m_Player.Downgrade();
                    if (!m_Player.IsDying()) {
                        m_Player.StartDamageInvincibility(2.0f);
                    }
                    LOG_INFO("Player hit by enemy! Downgrade triggered.");
                }
            }
        }
    }
}


// ─── CheckBlockCollision ──────────────────────────────────────────────────
void GameManager::CheckBlockCollision() {
    glm::vec2 pPos  = m_Player.GetPosition();
    glm::vec2 pPrev = m_Player.GetPreviousPosition();
    glm::vec2 pSize = m_Player.GetSize();
    glm::vec2 pVel  = m_Player.GetVelocity();
    bool onGround = false;

    std::vector<std::shared_ptr<Block>> blocksToRemove;

    for (const auto& block : m_Blocks) {
        if (!block->IsSolid()) continue;

        glm::vec2 bPos  = block->GetPosition();
        glm::vec2 bSize = block->GetSize();

        // 判斷 AABB 是否重疊
        if (CollisionUtils::CheckAABB(pPos, pSize, bPos, bSize)) {
            // 計算滲透深度來將瑪利歐擠出方塊
            float pCenterX = pPos.x + pSize.x * 0.5f;
            float pCenterY = pPos.y + pSize.y * 0.5f;
            float bCenterX = bPos.x + bSize.x * 0.5f;
            float bCenterY = bPos.y + bSize.y * 0.5f;

            float dx = pCenterX - bCenterX;
            float dy = pCenterY - bCenterY;

            float penX = (pSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
            float penY = (pSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

            constexpr float SURFACE_TOLERANCE = 2.0f;
            const float previousTop = pPrev.y;
            const float previousBottom = pPrev.y + pSize.y;
            const float currentBottom = pPos.y + pSize.y;
            const float overlapLeft = std::max(pPos.x, bPos.x);
            const float overlapRight = std::min(pPos.x + pSize.x, bPos.x + bSize.x);
            const float overlapX = std::max(0.0f, overlapRight - overlapLeft);
            const float blockTop = bPos.y;
            const float blockBottom = bPos.y + bSize.y;

            const bool landedFromAbove =
                pVel.y >= 0.0f &&
                previousBottom <= blockTop + SURFACE_TOLERANCE &&
                currentBottom >= blockTop;
            const bool standingOverlap =
                pVel.y >= 0.0f &&
                block->GetType() == Block::Type::Ground &&
                dy < 0.0f &&
                penY <= TILE_SIZE + 4.0f &&
                overlapX >= pSize.x * 0.5f;
            const bool hitFromBelow =
                pVel.y < 0.0f &&
                previousTop >= blockBottom - SURFACE_TOLERANCE &&
                pPos.y <= blockBottom;

            if (landedFromAbove || standingOverlap) {
                pPos.y -= penY;
                if (pVel.y > 0.0f) pVel.y = 0.0f;
                onGround = true;
                if (auto* platform = dynamic_cast<MovingPlatformBlock*>(block.get())) {
                    pPos += platform->GetFrameDelta();
                }
            } else if (hitFromBelow) {
                pPos.y += penY;
                pVel.y = 0.0f;

                BlockHitResult hitRes = block->OnHit(&m_Player);

                if (hitRes.isDestroyed) {
                    SpawnBrickDebris(bPos);
                    block->SetVisible(false);
                    blocksToRemove.push_back(block);
                }

                if (hitRes.spawnItem != "None") {
                    LOG_INFO("GameManager received Item Spawn request: {}", hitRes.spawnItem);
                    SpawnItem(hitRes.spawnItem, bPos);
                }
            } else {
                if (dx > 0.0f) pPos.x += penX;
                else           pPos.x -= penX;
            }
        }
    }

    m_Player.SetPosition(pPos);
    m_Player.SetVelocity(pVel);
    m_Player.SetOnGround(onGround);

    // 清理碎裂的方塊
    for (auto& b : blocksToRemove) {
        m_Blocks.erase(std::remove(m_Blocks.begin(), m_Blocks.end(), b), m_Blocks.end());
    }
}

void GameManager::CheckEnemyBlockCollision() {
    for (auto& enemy : m_Enemies) {
        if (!enemy->IsAlive()) continue;
        if (!enemy->UsesBlockCollision()) continue;

        glm::vec2 ePos  = enemy->GetPosition();
        glm::vec2 eSize = enemy->GetSize();
        glm::vec2 eVel  = enemy->GetVelocity();

        for (const auto& block : m_Blocks) {
            if (!block->IsSolid()) continue;

            const glm::vec2 bPos  = block->GetPosition();
            const glm::vec2 bSize = block->GetSize();

            // AABB 碰撞
            if (!CollisionUtils::CheckAABB(ePos, eSize, bPos, bSize)) continue;

            const float eCenterX = ePos.x + eSize.x * 0.5f;
            const float eCenterY = ePos.y + eSize.y * 0.5f;
            const float bCenterX = bPos.x + bSize.x * 0.5f;
            const float bCenterY = bPos.y + bSize.y * 0.5f;

            const float dx = eCenterX - bCenterX;
            const float dy = eCenterY - bCenterY;

            const float penX = (eSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
            const float penY = (eSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

            // 小於 4.0 像素的 Y 滲透通常是貼地滑行遇到的「方塊接縫 (Seam)」，強制當作 Y 軸處理
            if (penX < penY && penY > 4.0f) {
                if (dx > 0) ePos.x += penX;
                else        ePos.x -= penX;

                enemy->ReverseDirection();
                eVel.x = enemy->GetVelocity().x;
            } else {
                if (dy > 0) {
                    ePos.y += penY;
                    if (eVel.y < 0) eVel.y = 0.0f;
                } else {
                    ePos.y -= penY;
                    if (eVel.y > 0) eVel.y = 0.0f;
                }
            }
        }

        if (auto* koopa = dynamic_cast<Koopa*>(enemy.get())) {
            if (koopa->GetVariant() == Koopa::Variant::Red && !koopa->IsInShell()) {
                const float lookAheadX = eVel.x < 0.0f ? ePos.x - 2.0f : ePos.x + eSize.x + 2.0f;
                const glm::vec2 probePos = {lookAheadX, ePos.y + eSize.y + 1.0f};
                const glm::vec2 probeSize = {2.0f, 2.0f};
                bool hasSupportAhead = false;

                for (const auto& block : m_Blocks) {
                    if (!block->IsSolid()) continue;
                    if (CollisionUtils::CheckAABB(probePos, probeSize, block->GetPosition(), block->GetSize())) {
                        hasSupportAhead = true;
                        break;
                    }
                }

                if (!hasSupportAhead && eVel.y == 0.0f) {
                    enemy->ReverseDirection();
                    eVel = enemy->GetVelocity();
                }
            }
        }

        enemy->SetPosition(ePos);
        enemy->SetVelocity(eVel);
    }
}

// ─── BuildScene ───────────────────────────────────────────────────────────

void GameManager::BuildScene() {
    // 背景圖加在最前面（zIndex = 0，畫在最底層）
    if (m_Background) {
        m_Renderer.AddChild(m_Background);
    }

    // Player（用 no-op deleter 避免 shared_ptr 誤刪 stack 物件）
    m_Renderer.AddChild(std::shared_ptr<Player>(&m_Player, [](Player*){}));

    // 所有敵人
    for (auto& enemy : m_Enemies) m_Renderer.AddChild(enemy);

    // 所有方塊
    for (auto& block : m_Blocks)  m_Renderer.AddChild(block);

    // 關卡載入時已存在的道具，例如 1-3 空中金幣
    for (auto& item : m_Items) m_Renderer.AddChild(item);

    // 所有火球
    for (auto& fireball : m_Fireballs) m_Renderer.AddChild(fireball);

    // 磚塊碎片
    for (auto& debris : m_BrickDebris) m_Renderer.AddChild(debris);
}

// ─── SpawnItem ────────────────────────────────────────────────────────────
void GameManager::SpawnItem(const std::string& itemType, glm::vec2 position) {
    std::shared_ptr<Item> newItem;

    if (itemType == "Coin") {
        newItem = std::make_shared<CoinItem>(position);
        m_Session.AddCoin();
    } else if (itemType == "PowerUp" || itemType == "Mushroom") {
        if (m_Player.GetForm() == Player::Form::SMALL) {
            newItem = std::make_shared<MushroomItem>(position);
        } else {
            newItem = std::make_shared<FireFlowerItem>(position);
        }
    } else if (itemType == "FireFlower") {
        newItem = std::make_shared<FireFlowerItem>(position);
    } else if (itemType == "OneUp" || itemType == "1Up") {
        newItem = std::make_shared<OneUpMushroomItem>(position);
    } else if (itemType == "Star" || itemType == "Starman") {
        newItem = std::make_shared<StarmanItem>(position);
    }

    if (newItem) {
        m_Items.push_back(newItem);
        m_Renderer.AddChild(newItem);
    }
}

// ─── CheckItemCollision ───────────────────────────────────────────────────
void GameManager::CheckItemCollision() {
    glm::vec2 pPos  = m_Player.GetPosition();
    glm::vec2 pSize = m_Player.GetSize();

    std::vector<std::shared_ptr<Item>> itemsToRemove;

    for (auto& item : m_Items) {
        // 香菇只有在 Active 狀態才能吃
        // 金幣不透過接觸吃，但金幣生命週期結束變成 Collected 也會移除
        if (item->GetState() == ItemState::Active && item->GetType() != "Coin") {
            glm::vec2 iPos  = item->GetPosition();
            glm::vec2 iSize = item->GetSize();

            if (CollisionUtils::CheckAABB(pPos, pSize, iPos, iSize)) {
                // 吃掉！
                const std::string itemType = item->GetType();
                item->OnCollect(&m_Player);
                if (item->GetState() == ItemState::Collected) {
                    if (itemType == "LevelCoin") {
                        m_Session.AddCoin();
                        m_Session.AddScore(200);
                    } else if (itemType == "OneUp") {
                        m_Session.AddLife();
                    } else if (itemType == "Mushroom" || itemType == "FireFlower" || itemType == "Star") {
                        m_Session.AddScore(1000);
                    }
                }
            }
        }

        // 把狀態變為 Collected 的項目加入移除名單
        if (item->GetState() == ItemState::Collected) {
            item->SetVisible(false);
            itemsToRemove.push_back(item);
        }
    }

    for (auto& i : itemsToRemove) {
        m_Items.erase(std::remove(m_Items.begin(), m_Items.end(), i), m_Items.end());
    }
}

// ─── CheckItemBlockCollision ──────────────────────────────────────────────────
void GameManager::CheckItemBlockCollision() {
    for (auto& item : m_Items) {
        if (item->GetState() != ItemState::Active) continue;
        if (item->GetType() == "Coin") continue; // 金幣無視地形物理跑動畫

        glm::vec2 iPos  = item->GetPosition();
        glm::vec2 iSize = item->GetSize();
        glm::vec2 iVel  = item->GetVelocity();

        for (const auto& block : m_Blocks) {
            if (!block->IsSolid()) continue;

            const glm::vec2 bPos  = block->GetPosition();
            const glm::vec2 bSize = block->GetSize();

            // 檢查是否碰到方塊
            if (!CollisionUtils::CheckAABB(iPos, iSize, bPos, bSize)) continue;

            const float iCenterX = iPos.x + iSize.x * 0.5f;
            const float iCenterY = iPos.y + iSize.y * 0.5f;
            const float bCenterX = bPos.x + bSize.x * 0.5f;
            const float bCenterY = bPos.y + bSize.y * 0.5f;

            const float dx = iCenterX - bCenterX;
            const float dy = iCenterY - bCenterY;

            const float penX = (iSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
            const float penY = (iSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

            // 小於 4.0 像素的 Y 滲透通常是貼地滑行遇到的「方塊接縫 (Seam)」，強制當作 Y 軸處理
            if (penX < penY && penY > 4.0f) {
                if (dx > 0) iPos.x += penX;
                else        iPos.x -= penX;

                iVel.x = -iVel.x; // 香菇撞牆反彈
            } else {
                if (dy > 0) {
                    iPos.y += penY;
                    if (iVel.y < 0) iVel.y = 0.0f;
                } else {
                    iPos.y -= penY;
                    if (iVel.y > 0) {
                        if (item->GetType() == "Star") {
                            iVel.y = -220.0f;
                        } else {
                            iVel.y = 0.0f;
                        }
                    }
                }
            }
        }

        item->SetPosition(iPos);
        item->SetVelocity(iVel);
    }
}

// ─── SpawnFireball ────────────────────────────────────────────────────────
void GameManager::SpawnFireball(glm::vec2 position, bool movingLeft) {
    auto fireball = std::make_shared<Fireball>(position, movingLeft);
    m_Fireballs.push_back(fireball);
    m_Renderer.AddChild(fireball);
    LOG_INFO("GameManager spawned Fireball at {}", position);
}

void GameManager::SpawnBrickDebris(glm::vec2 position) {
    const glm::vec2 center = position + glm::vec2(TILE_SIZE * 0.5f, TILE_SIZE * 0.5f);
    const std::array<glm::vec2, 4> offsets = {
        glm::vec2(-4.0f, -4.0f),
        glm::vec2(4.0f, -4.0f),
        glm::vec2(-4.0f, 4.0f),
        glm::vec2(4.0f, 4.0f),
    };
    const std::array<glm::vec2, 4> velocities = {
        glm::vec2(-90.0f, -280.0f),
        glm::vec2(90.0f, -280.0f),
        glm::vec2(-70.0f, -190.0f),
        glm::vec2(70.0f, -190.0f),
    };

    for (std::size_t i = 0; i < offsets.size(); ++i) {
        auto debris = std::make_shared<BrickDebris>(center + offsets[i], velocities[i], m_Level.theme);
        m_BrickDebris.push_back(debris);
        m_Renderer.AddChild(debris);
    }
}

// ─── CheckFireballCollision ───────────────────────────────────────────────
void GameManager::CheckFireballCollision() {
    std::vector<std::shared_ptr<Fireball>> fireballsToRemove;

    for (auto& fireball : m_Fireballs) {
        if (fireball->IsDead()) {
            fireballsToRemove.push_back(fireball);
            continue;
        }

        // 當火球正在播放碎片爆炸動畫時，不再計算碰撞
        if (fireball->IsExploded()) {
            continue;
        }

        glm::vec2 fPos  = fireball->GetPosition();
        glm::vec2 fSize = fireball->GetSize();
        glm::vec2 fVel  = fireball->GetVelocity();
        bool collidedThisFrame = false;

        // 1. 與方塊的碰撞
        for (const auto& block : m_Blocks) {
            if (!block->IsSolid()) continue;

            const glm::vec2 bPos  = block->GetPosition();
            const glm::vec2 bSize = block->GetSize();

            // AABB 判定：火球 vs 方塊
            if (!CollisionUtils::CheckAABB(fPos, fSize, bPos, bSize)) continue;

            const float fCenterX = fPos.x + fSize.x * 0.5f;
            const float fCenterY = fPos.y + fSize.y * 0.5f;
            const float bCenterX = bPos.x + bSize.x * 0.5f;
            const float bCenterY = bPos.y + bSize.y * 0.5f;

            const float dx = fCenterX - bCenterX;
            const float dy = fCenterY - bCenterY;

            const float penX = (fSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
            const float penY = (fSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

            if (penX < penY) {
                // X 軸推擠，撞到牆 -> 爆炸
                fireball->Explode();
                collidedThisFrame = true;
                break;
            }

            if (dy < 0.0f && fVel.y > 0.0f) {
                // 從上方落到地板/方塊頂面，推出碰撞後反彈。
                fPos.y -= penY;
                fireball->SetPosition(fPos);
                fireball->Bounce();
                fVel = fireball->GetVelocity();
            } else {
                // 撞到方塊底部或非下落狀態撞上垂直面，避免貼著天花板滑行。
                fireball->Explode();
                collidedThisFrame = true;
                break;
            }
        }

        if (collidedThisFrame) continue;

        // 2. 與敵人的碰撞
        for (auto& enemy : m_Enemies) {
            if (!enemy->IsAlive()) continue;

            glm::vec2 ePos  = enemy->GetPosition();
            glm::vec2 eSize = enemy->GetSize();

            // 火球 vs 敵人 AABB 碰撞
            if (CollisionUtils::CheckAABB(fPos, fSize, ePos, eSize)) {
                // 擊中敵人！
                enemy->SetAlive(false);
                enemy->SetVisible(false);
                fireball->Explode();
                LOG_INFO("Fireball killed an enemy!");
                break;
            }
        }
        
        // 3. 虛空判定
        const float killZ = m_Level.levelHeight + 50.0f;
        if (fPos.y > killZ) {
            fireball->Explode();
        }
    }

    // 移除死掉的火球
    for (auto& fb : fireballsToRemove) {
        m_Fireballs.erase(std::remove(m_Fireballs.begin(), m_Fireballs.end(), fb), m_Fireballs.end());
    }
}

// ─── CheckFlagCollision ───────────────────────────────────────────────────

/*
 * 終點旗杆碰撞偵測
 *
 * 掃描所有 Block，找出 Type::Flag 的旗杆。
 * 用 AABB 判斷玩家是否進入旗杆範圍，若是：
 *   1. 呼叫 GetContactScore(playerY) 計算分數（依接觸高度）
 *   2. 把分數加入目前玩家進度
 *   3. 標記過關（避免重複觸發）
 */
void GameManager::CheckFlagCollision() {
    if (!m_Player.IsAlive())  return;
    if (m_LevelCleared)       return; // 已過關，不重複計算

    const glm::vec2 pPos  = m_Player.GetPosition();
    const glm::vec2 pSize = m_Player.GetSize();

    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Flag) continue;

        const glm::vec2 fPos  = block->GetPosition();
        const glm::vec2 fSize = block->GetSize();

        // 旗子本體的碰撞框 (AABB)
        if (!CollisionUtils::CheckAABB(pPos, pSize, fPos, fSize)) continue;

        // ── 碰到旗杆！──
        // 用玩家腳底 Y 計算接觸高度
        const float playerBottom = pPos.y + pSize.y;

        // dynamic_cast 取得 FlagBlock 指標以呼叫 GetContactScore
        auto* flagBlock = dynamic_cast<FlagBlock*>(block.get());
        if (!flagBlock) continue;

        const int score = flagBlock->GetContactScore(playerBottom);
        m_Session.AddScore(score);
        m_LevelCleared = true;
        
        // 觸發玩家的過關下降演出
        m_Player.StartLevelClearSequence(fPos.x, fPos.y + fSize.y);

        LOG_INFO("=== LEVEL CLEAR! Flag score: {}  Total score: {} ===",
                 score,
                 m_Session.CurrentPlayer().score);
        // TODO: 播放過關音樂、觸發旗子下降動畫、延遲幾秒後進下一關
        break;
    }
}
