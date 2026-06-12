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
// runtime 關卡 JSON 統一放在 Resources/data/
static const std::string kDefaultInitialLevelName = "1-1";

// ─── 關卡鏈定義（Batch A）────────────────────────────────────────────────
// 過關順序：1-1 → 1-2 → 1-3 → 回標題
// 新增關卡只需在此陣列末尾追加元素
const std::vector<GameManager::LevelEntry> GameManager::kLevelChain = {
    {"1-1",          "1-1"},
    {"1-2_ground_1", "1-2"},
    {"1-3_ground_1", "1-3"},
};

// ─── Combo 分數序列（NES Mario 踩踏 / 殼連殺）────────────────────────────
// 第 n 次連殺的分值：100, 200, 400, 500, 800, 1000, 2000, 4000, 5000, 1UP...
static const std::array<int, 9> kComboScores = {
    100, 200, 400, 500, 800, 1000, 2000, 4000, 5000
};

// 返回當前 combo 計數對應的分數，-1 表示 1UP（額外命）
// 同時將 m_ComboCount 向前推進一步
int GameManager::NextComboScore() {
    int idx = m_ComboCount;
    ++m_ComboCount;
    if (idx < static_cast<int>(kComboScores.size())) {
        return kComboScores[idx];
    }
    return -1; // 1UP
}

// 殼連殺計分：使用同一個 combo 分數表
int GameManager::NextShellComboScore() {
    int idx = m_ShellComboCount;
    ++m_ShellComboCount;
    if (idx < static_cast<int>(kComboScores.size())) {
        return kComboScores[idx];
    }
    return -1; // 1UP
}

void GameManager::ResetCombo() {
    m_ComboCount = 0;
}

// ─── 浮動得分彈出 ─────────────────────────────────────────────────────────
void GameManager::SpawnScorePopup(int score, glm::vec2 worldPos) {
    const std::string fontPath = MakeAssetPath("font/Super Mario Bros. NES.ttf");
    std::string label = (score < 0) ? "1UP" : std::to_string(score);

    auto text = std::make_shared<Util::Text>(
        fontPath, 12, label, Util::Color(255, 255, 255));

    auto obj = std::make_shared<Util::GameObject>(text, 25.0f);
    obj->m_Transform.translation = m_Camera.WorldToScreen(worldPos);
    obj->SetVisible(true);

    m_Renderer.AddChild(obj);

    ScorePopup popup;
    popup.obj      = obj;
    popup.worldPos = worldPos;
    popup.timer    = 0.0f;
    popup.totalLife = 0.8f;
    m_ScorePopups.push_back(std::move(popup));
}

void GameManager::UpdateScorePopups(float dt) {
    constexpr float RISE_SPEED = 30.0f; // 世界像素 / 秒，往上漂移

    for (auto& p : m_ScorePopups) {
        p.timer += dt;
        // 往上漂移（世界座標 Y 減少 = 向上）
        p.worldPos.y -= RISE_SPEED * dt;
        // 每幀以世界座標換算螢幕座標
        p.obj->m_Transform.translation = m_Camera.WorldToScreen(p.worldPos);

        if (p.timer >= p.totalLife) {
            p.obj->SetVisible(false);
        }
    }

    // 移除已結束的彈出
    m_ScorePopups.erase(
        std::remove_if(m_ScorePopups.begin(), m_ScorePopups.end(),
                       [](const ScorePopup& p) { return p.timer >= p.totalLife; }),
        m_ScorePopups.end());
}

namespace {
constexpr float kTitleBackdropZ = 25.0f;
constexpr float kTitleTextZ = 30.0f;

glm::vec2 GetPipeSize(const std::string& opening, int segments) {
    const float clampedSegments = static_cast<float>(std::max(1, segments));
    if (opening == "left" || opening == "right") {
        return {TILE_SIZE * clampedSegments, TILE_SIZE * 2.0f};
    }
    return {TILE_SIZE * 2.0f, TILE_SIZE * clampedSegments};
}

glm::vec2 GetPipePositionFromAnchor(glm::vec2 anchorPosition,
                                    const std::string& opening,
                                    int segments) {
    const int clampedSegments = std::max(1, segments);
    if (opening == "down") {
        return {anchorPosition.x, anchorPosition.y - (clampedSegments - 1) * TILE_SIZE};
    }
    if (opening == "right") {
        return {anchorPosition.x - (clampedSegments - 1) * TILE_SIZE, anchorPosition.y};
    }
    return anchorPosition;
}
}

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
        case FlowState::GameOver:
            UpdateGameOver(dt);
            break;
        case FlowState::LevelClearTransition:
            UpdateLevelClearTransition(dt);
            break;
        case FlowState::LevelClearPause:
            UpdateLevelClearPause(dt);
            break;
    }
}

void GameManager::UpdateTitle(float dt) {
    (void)dt;

    if (Util::Input::IsKeyDown(Util::Keycode::NUM_4)) {
        SelectInitialLevel("1-1", "1-1");
        DrawScene(false);
        return;
    }
    if (Util::Input::IsKeyDown(Util::Keycode::NUM_5)) {
        SelectInitialLevel("1-2_ground_1", "1-2");
        DrawScene(false);
        return;
    }
    if (Util::Input::IsKeyDown(Util::Keycode::NUM_6)) {
        SelectInitialLevel("1-3_ground_1", "1-3");
        DrawScene(false);
        return;
    }

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

    DrawScene(true);
}

void GameManager::UpdatePlaying(float dt) {
    // ─── 暫停切換（Enter 鍵，單幀偵測防連發）──────────────────────────────
    // Note：Title 畫面的 Enter 在 UpdateTitle 裡處理，不會到達這裡
    if (Util::Input::IsKeyDown(Util::Keycode::RETURN)) {
        m_Paused = !m_Paused;

        if (m_Paused) {
            // 建立 PAUSED overlay（加入 renderer）
            if (!m_PauseOverlay) {
                const std::string fontPath = MakeAssetPath("font/Super Mario Bros. NES.ttf");
                auto context = Core::Context::GetInstance();
                const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;
                auto text = std::make_shared<Util::Text>(
                    fontPath, 24, "PAUSED", Util::Color(255, 255, 255));
                m_PauseOverlay = std::make_shared<Util::GameObject>(text, 35.0f);
                m_PauseOverlay->m_Transform.translation = {0.0f, halfH - 200.0f};
                m_PauseOverlay->SetVisible(true);
                m_Renderer.AddChild(m_PauseOverlay);
            } else {
                m_PauseOverlay->SetVisible(true);
            }
            LOG_INFO("Game paused.");
        } else {
            if (m_PauseOverlay) m_PauseOverlay->SetVisible(false);
            LOG_INFO("Game resumed.");
        }
    }

    // 暫停時只更新渲染，不更新任何物件或時間
    if (m_Paused) {
        m_Renderer.Update();
        return;
    }

    if (m_Player.GetState() == Player::State::EnteringPipe && m_Player.IsAnimationFinished()) {
        ChangeLevel(m_PendingLevel, m_PendingSpawn);
        return;
    }

    // 1. 更新所有角色邏輯
    m_Player.Update(dt);

    // ── 跳躍音效：Player 在 Jump() 設旗，GameManager 消耗並播音 ──
    if (m_Player.ConsumeJumpEvent()) {
        const std::string jumpSfx = (m_Player.GetForm() == Player::Form::SMALL)
                                        ? "jump_small" : "jump_super";
        m_Audio.PlaySFX(jumpSfx);
    }

    // ── 星星效果結束：切回關卡 BGM ──
    if (m_Player.ConsumeStarEndedEvent()) {
        m_Audio.PlayBGM(m_LevelBGMName);
        LOG_INFO("Star ended, restoring level BGM: {}", m_LevelBGMName);
    }

    // ── 死亡偵測：玩家剛進入 Dying 狀態時，停關卡 BGM 改播死亡 BGM ──
    const bool playerIsDyingNow = m_Player.IsDying();
    if (playerIsDyingNow && !m_PlayerWasDying) {
        m_Audio.StopBGM();
        m_Audio.PlayBGM("death", 0); // 死亡 BGM 播一次
    }
    m_PlayerWasDying = playerIsDyingNow;

    // 變身動畫結束時套用待定的傷害無敵（縮小後才開始計時）
    if (!m_Player.IsTransforming() && m_PendingDamageInvincibility > 0.0f) {
        m_Player.StartDamageInvincibility(m_PendingDamageInvincibility);
        m_PendingDamageInvincibility = 0.0f;
        LOG_INFO("Post-transform damage invincibility started.");
    }

    if (m_Player.GetState() == Player::State::EnteringPipe || m_Player.GetState() == Player::State::ExitingPipe) {
        m_Camera.Update(m_Player.GetPosition().x,
                        static_cast<float>(m_Level.levelWidth));
        DrawScene(true);
        return;
    }

    // 變身動畫中：全場凍結（不更新敵人/方塊/道具/時間），
    // 只讓玩家自己更新（動畫已在 m_Player.Update(dt) 裡處理）
    if (m_Player.IsTransforming()) {
        m_Camera.Update(m_Player.GetPosition().x,
                        static_cast<float>(m_Level.levelWidth));
        DrawScene(true);
        return;
    }

    // 如果瑪利歐正在播放死亡動畫，停止更新世界其他物件（凍結畫面）
    if (!m_Player.IsDying()) {
        const float playerX = m_Player.GetPosition().x + m_Player.GetSize().x * 0.5f;
        for (auto& enemy : m_Enemies) {
            // 翻轉死亡中的 Koopa 仍需要更新物理（飛出畫面）
            auto* koopa = dynamic_cast<Koopa*>(enemy.get());
            if (koopa && koopa->IsDying()) {
                koopa->Update(dt);
                continue;
            }
            // 食人花：每幀同步玩家 X 座標
            auto* piranha = dynamic_cast<PiranhaPlant*>(enemy.get());
            if (piranha) {
                piranha->SetPlayerX(playerX);
            }
            enemy->Update(dt);
        }
        for (auto& block : m_Blocks)  block->Update(dt);
        for (auto& item : m_Items)    item->Update(dt);
        for (auto& fireball : m_Fireballs) fireball->Update(dt);
        for (auto& debris : m_BrickDebris) debris->Update(dt);
        UpdateScorePopups(dt);

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
            CheckShellEnemyCollision();
            CheckFireballCollision();
            CheckFlagCollision();
        }

        // 4. 玩家左界夾制：確保玩家不會走到鏡頭左邊界之外（棘輪規則的配套）
        m_Player.ClampToCameraBounds(m_Camera.GetX());

        // 5. 中繼點偵測：更新玩家已達成的最後中繼點
        if (m_Player.IsAlive()) {
            UpdateCheckpoints();
        }
    }

    // ─── 虛空掉落判定 (Kill Z) ───
    const float killZ = m_Level.levelHeight + 50.0f;

    // Time Up 等待死亡動畫時：玩家落出畫面後進 TIME UP overlay，不走一般生命扣除
    if (m_WaitingForTimeUpDeath) {
        if (m_Player.GetPosition().y > killZ) {
            m_WaitingForTimeUpDeath = false;
            EnterTimeUp();
            return;
        }
        DrawScene(true);
        return;
    }

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

    // ─── 倒數計時 ─────────────────────────────────────────────────────────
    if (!m_LevelCleared && m_Player.IsAlive()) {
        m_TimeRemaining -= dt * 2.5f;
        if (m_TimeRemaining <= 0.0f) {
            m_TimeRemaining = 0.0f;
            // Time Up：先觸發玩家死亡動畫，再進入 TIME UP overlay
            // 原版 NES：TIME UP 時不降級，直接死亡彈跳
            // 強制設為 SMALL 再 Downgrade → 進入 Dying state
            m_WaitingForTimeUpDeath = true;
            m_Player.SetForm(Player::Form::SMALL);
            m_Player.Downgrade();  // SMALL -> Dying state（含死亡彈跳動畫）
            LOG_INFO("Time Up! Player death animation started.");
            DrawScene(true);
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

void GameManager::UpdateGameOver(float dt) {
    m_StateTimer += dt;
    // GAME OVER 畫面顯示約 3 秒後回標題
    if (m_StateTimer >= 3.0f) {
        EnterTitleScreen();
        DrawScene(false);
        return;
    }

    m_Renderer.Update();
}

// 時間結算倒數：每秒固定扣約 N 單位時間並同步加分
// NES 原版：約每幀扣 2 單位（30fps * 2 = 60/s），這裡用 dt 換算
void GameManager::UpdateLevelClearTransition(float dt) {
    constexpr float TIME_DRAIN_PER_SECOND = 60.0f; // 每秒扣幾單位
    constexpr float SCORE_PER_UNIT = 50.0f;        // 每單位加 50 分

    if (m_TimeRemaining > 0.0f) {
        m_CountdownAccum += dt * TIME_DRAIN_PER_SECOND;
        const int unitsToDeduct = static_cast<int>(m_CountdownAccum);
        if (unitsToDeduct > 0) {
            m_CountdownAccum -= static_cast<float>(unitsToDeduct);

            const int actualDeduct = std::min(unitsToDeduct, static_cast<int>(m_TimeRemaining));
            m_TimeRemaining -= static_cast<float>(actualDeduct);
            m_Session.AddScore(actualDeduct * static_cast<int>(SCORE_PER_UNIT));

            if (m_TimeRemaining <= 0.0f) {
                m_TimeRemaining = 0.0f;
            }
        }
    } else {
        // 時間已歸零，進入停頓狀態
        EnterLevelClearPause();
        return;
    }

    // 每幀更新畫面（含 HUD 時間與分數的即時動態顯示）
    DrawScene(true);
}

// 結算結束後停頓 1 秒，再進下一關
void GameManager::UpdateLevelClearPause(float dt) {
    m_StateTimer += dt;
    if (m_StateTimer >= 1.0f) {
        AdvanceToNextLevel();
        return;
    }
    DrawScene(true);
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
    m_ScorePopups.clear();
    // 效能優化：清空複用的暫存容器（不釋放已分配的 capacity）
    m_TmpBlocksToRemove.clear();
    m_TmpItemsToRemove.clear();
    m_TmpFireballsToRemove.clear();
    m_Background.reset();
    m_Renderer = Util::Renderer();
    ResetCombo();
}

void GameManager::StartNewGame() {
    ResetSceneObjects();
    m_Session.ResetNewGame(1);
    m_Player.ResetForNewGame();

    // 新遊戲：清空中繼點與中繼點重生覆蓋
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;

    // 找到標題畫面選的關卡在關卡鏈中的位置
    m_CurrentLevelIndex = 0; // 預設從頭
    for (int i = 0; i < static_cast<int>(kLevelChain.size()); ++i) {
        if (kLevelChain[i].levelName == m_SelectedInitialLevelName) {
            m_CurrentLevelIndex = i;
            break;
        }
    }

    // 套用關卡鏈的關卡名稱到目前關卡欄位
    m_SelectedWorldLabel = kLevelChain[m_CurrentLevelIndex].worldLabel;
    // 同步進度
    m_Session.CurrentPlayer().levelName = kLevelChain[m_CurrentLevelIndex].levelName;

    EnterLevelIntro();
    LOG_INFO("New game started. Level index={} name='{}' world='{}'",
             m_CurrentLevelIndex,
             kLevelChain[m_CurrentLevelIndex].levelName,
             m_SelectedWorldLabel);
}

void GameManager::EnterLevelIntro() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 400.0f;
    m_LevelCleared = false;
    m_WaitingForTimeUpDeath = false;
    m_Paused = false;          // 換關時清空暫停
    m_PauseOverlay = nullptr;  // overlay 隨 renderer 重建
    m_PendingDamageInvincibility = 0.0f; // 清空待定無敵

    // 決定要載入的關卡：若 m_CurrentLevelIndex 有效則用關卡鏈，否則退回標題選關
    const std::string levelToLoad = (m_CurrentLevelIndex >= 0 &&
                                     m_CurrentLevelIndex < static_cast<int>(kLevelChain.size()))
                                    ? kLevelChain[m_CurrentLevelIndex].levelName
                                    : m_SelectedInitialLevelName;

    // 同步 world label（確保 HUD 顯示正確關卡名）
    if (m_CurrentLevelIndex >= 0 && m_CurrentLevelIndex < static_cast<int>(kLevelChain.size())) {
        m_SelectedWorldLabel = kLevelChain[m_CurrentLevelIndex].worldLabel;
    }

    LoadLevel(MakeLevelPath(levelToLoad));
    ApplyPlayerProgress();

    // 若有中繼點重生覆蓋，套用後清除（只用一次）
    if (m_CheckpointRespawnOverride.has_value()) {
        m_Player.SetSpawnPosition(*m_CheckpointRespawnOverride);
        LOG_INFO("Checkpoint respawn override applied: x={} y={}",
                 m_CheckpointRespawnOverride->x,
                 m_CheckpointRespawnOverride->y);
        m_CheckpointRespawnOverride = std::nullopt;
    }

    {
        const float lvW = static_cast<float>(m_Level.levelWidth);
        m_Camera.SetX(std::clamp(
            m_Player.GetPosition().x - m_Camera.GetViewWorldWidth() * 0.5f,
            0.0f, std::max(0.0f, lvW - m_Camera.GetViewWorldWidth())));
    }
    BuildScene();
    m_Hud.Init(m_Renderer, m_SelectedWorldLabel);
    const auto& progress = m_Session.CurrentPlayer();
    m_Hud.Update(progress.score, progress.coins, static_cast<int>(m_TimeRemaining));
    BuildLevelIntroOverlay();

    m_FlowState = FlowState::LevelIntro;
    LOG_INFO("Entered level intro: player='{}' lives={}",
             m_Session.GetCurrentPlayerName(),
             progress.lives);
}

void GameManager::EnterPlaying() {
    // 注意：EnterPlaying 由 UpdateLevelIntro 呼叫，
    // 此時關卡已在 EnterLevelIntro 完整載入（含中繼點重生座標）。
    // 不重新 LoadLevel，避免覆蓋 EnterLevelIntro 中已套用的 checkpoint spawn。
    // 只需重置遊戲邏輯狀態、重建場景並開始播 BGM 即可。

    // 重建 renderer（移除 intro overlay，重建可玩場景）
    m_Renderer = Util::Renderer();
    m_OverlayObjects.clear();
    m_ScorePopups.clear();
    // 重建敵人/方塊/道具的 renderer 子節點（物件本身不清空，保留關卡狀態）
    BuildScene();
    m_Hud.Init(m_Renderer, m_SelectedWorldLabel);

    m_LevelCleared = false;
    m_WaitingForTimeUpDeath = false;
    m_Paused = false;          // 確保進入遊玩狀態時不殘留暫停
    m_PauseOverlay = nullptr;  // overlay 隨 renderer 重建
    m_PendingDamageInvincibility = 0.0f; // 清空待定無敵

    m_FlowState = FlowState::Playing;
    m_PlayerWasDying = false; // 重置死亡偵測狀態
    // 開始播放關卡 BGM（依 LoadLevel 時決定的主題）
    m_Audio.PlayBGM(m_LevelBGMName);
    LOG_INFO("Entered playing state.");
}

void GameManager::EnterTitleScreen() {
    ResetSceneObjects();
    m_LevelCleared = false;
    m_WaitingForTimeUpDeath = false;
    m_StateTimer = 0.0f;
    m_LevelClearTransitionTimer = 0.0f;
    m_Player.ResetForNewGame();
    // 回標題畫面時停止所有音樂
    m_Audio.StopBGM();

    LoadLevel(MakeLevelPath(m_SelectedInitialLevelName.empty() ? kDefaultInitialLevelName : m_SelectedInitialLevelName));
    {
        const float lvW = static_cast<float>(m_Level.levelWidth);
        m_Camera.SetX(std::clamp(
            m_Player.GetPosition().x - m_Camera.GetViewWorldWidth() * 0.5f,
            0.0f, std::max(0.0f, lvW - m_Camera.GetViewWorldWidth())));
    }
    BuildScene();
    BuildTitleOverlay();

    m_FlowState = FlowState::Title;
    LOG_INFO("Entered title screen.");
}

void GameManager::SelectInitialLevel(const std::string& levelName, const std::string& worldLabel) {
    if (m_SelectedInitialLevelName == levelName && m_SelectedWorldLabel == worldLabel) {
        return;
    }

    m_SelectedInitialLevelName = levelName;
    m_SelectedWorldLabel = worldLabel;

    ResetSceneObjects();
    m_Player.ResetForNewGame();
    LoadLevel(MakeLevelPath(m_SelectedInitialLevelName));
    {
        const float lvW = static_cast<float>(m_Level.levelWidth);
        m_Camera.SetX(std::clamp(
            m_Player.GetPosition().x - m_Camera.GetViewWorldWidth() * 0.5f,
            0.0f, std::max(0.0f, lvW - m_Camera.GetViewWorldWidth())));
    }
    BuildScene();
    BuildTitleOverlay();
    m_FlowState = FlowState::Title;

    LOG_INFO("Selected initial level: '{}' ({})", m_SelectedInitialLevelName, m_SelectedWorldLabel);
}

void GameManager::EnterTimeUp() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 0.0f;
    m_LevelCleared = false;

    m_Hud.Init(m_Renderer, m_SelectedWorldLabel);
    const auto& progress = m_Session.CurrentPlayer();
    m_Hud.Update(progress.score, progress.coins, 0);
    BuildTimeUpOverlay();

    m_FlowState = FlowState::TimeUp;
    LOG_INFO("Entered time up state.");
}

void GameManager::EnterGameOver() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_LevelCleared = false;
    m_WaitingForTimeUpDeath = false;

    // 建立黑底 + GAME OVER 文字 overlay
    BuildGameOverOverlay();

    m_FlowState = FlowState::GameOver;
    // 播放 Game Over 音效 BGM（播一次，不循環）
    m_Audio.PlayBGM("game_over", 0);
    LOG_INFO("Entered GAME OVER state.");
}

void GameManager::EnterLevelClearTransition() {
    SavePlayerProgress();
    // 不在此清空場景——保留畫面背景，等時間結算結束後再切關
    // （ResetSceneObjects 會在進下一關 EnterLevelIntro 時呼叫）
    m_LevelClearTransitionTimer = 0.0f;
    m_CountdownAccum = 0.0f;
    BuildLevelClearOverlay();
    m_FlowState = FlowState::LevelClearTransition;
    // 播放過關 BGM（播一次，不循環）
    m_Audio.PlayBGM("level_clear", 0);
    LOG_INFO("Entered level clear transition (time countdown). timeRemaining={}",
             m_TimeRemaining);
}

void GameManager::EnterLevelClearPause() {
    m_StateTimer = 0.0f;
    m_FlowState = FlowState::LevelClearPause;
    LOG_INFO("Entered level clear pause. Proceeding to next level in 1s.");
}

// 推進到下一關，或若已是最後一關則回標題
void GameManager::AdvanceToNextLevel() {
    const int nextIndex = m_CurrentLevelIndex + 1;

    if (nextIndex >= static_cast<int>(kLevelChain.size())) {
        // 已是最後一關，回標題
        LOG_INFO("All levels cleared! Returning to title.");
        // 重置進度 index，讓下次開始新遊戲從第一關開始
        m_CurrentLevelIndex = -1;
        EnterTitleScreen();
        return;
    }

    // 推進到下一關
    m_CurrentLevelIndex = nextIndex;
    m_SelectedWorldLabel = kLevelChain[m_CurrentLevelIndex].worldLabel;
    m_Session.CurrentPlayer().levelName = kLevelChain[m_CurrentLevelIndex].levelName;

    // 換關時清空中繼點（新關卡從頭開始）
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;

    LOG_INFO("Advancing to level index={} name='{}' world='{}'",
             m_CurrentLevelIndex,
             kLevelChain[m_CurrentLevelIndex].levelName,
             m_SelectedWorldLabel);

    EnterLevelIntro();
}

void GameManager::BuildTitleOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfW = static_cast<float>(context->GetWindowWidth()) * 0.5f;
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    AddOverlayImage(
        "ui/title/black.png",
        {0.0f, 0.0f},
        {halfW * 2.0f, halfH * 2.0f},
        kTitleBackdropZ);

    AddOverlayImage("ui/title/logo.png", {0.0f, halfH - 150.0f}, {3.0f, 3.0f}, kTitleTextZ);
    AddOverlayText("TOP - 000000", 14, {0.0f, halfH - 270.0f}, kTitleTextZ);
    AddOverlayText("1 PLAYER GAME", 16, {0.0f, halfH - 335.0f}, kTitleTextZ);
    AddOverlayText("WORLD " + m_SelectedWorldLabel, 14, {0.0f, halfH - 380.0f}, kTitleTextZ);
    AddOverlayText("4:1-1  5:1-2  6:1-3", 12, {0.0f, halfH - 420.0f}, kTitleTextZ);
    AddOverlayText("PRESS ENTER OR SPACE", 12, {0.0f, halfH - 465.0f}, kTitleTextZ);
}

void GameManager::BuildLevelIntroOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    AddOverlayText("WORLD  " + m_SelectedWorldLabel, 22, {0.0f, halfH - 260.0f});

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

void GameManager::BuildGameOverOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    // 仿 NES 原版：黑底居中顯示 GAME OVER
    AddOverlayText("GAME OVER", 28, {0.0f, halfH - 400.0f});
}

void GameManager::BuildLevelClearOverlay() {
    const auto context = Core::Context::GetInstance();
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    // 顯示過關訊息與時間結算提示（NES 風格）
    AddOverlayText("WORLD CLEAR!", 24, {0.0f, halfH - 160.0f});
    AddOverlayText("TIME BONUS", 18, {0.0f, halfH - 240.0f});
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
    m_Session.LoseLife();
    m_Session.CurrentPlayer().form = Player::Form::SMALL;

    if (m_Session.IsGameOver()) {
        LOG_INFO("Game over. Showing GAME OVER screen.");
        // 清空中繼點（Game Over 後回到標題，不保留進度）
        m_LastCheckpoint = std::nullopt;
        EnterGameOver();
        return;
    }

    m_Session.SwitchToNextAlivePlayer();

    // 若有達成的中繼點，重生位置改為中繼點（保持存活期間不清空）
    if (m_LastCheckpoint.has_value()) {
        LOG_INFO("Respawning at checkpoint: x={} y={}",
                 m_LastCheckpoint->x, m_LastCheckpoint->y);
        // 暫存中繼點座標，EnterLevelIntro 呼叫 LoadLevel 後覆蓋 playerSpawn
        m_CheckpointRespawnOverride = m_LastCheckpoint;
    } else {
        m_CheckpointRespawnOverride = std::nullopt;
    }

    EnterLevelIntro();
}

// ─── LoadLevel ────────────────────────────────────────────────────────────

void GameManager::LoadLevel(const std::string& jsonPath) {
    m_Level = LevelLoader::Load(jsonPath);
    m_ThemeAssets = ThemeAssets(m_Level.theme);

    LOG_INFO(
        "Level loaded: json='{}' background='{}' levelSize=({}, {}) playerSpawn={}",
        jsonPath,
        m_Level.backgroundImagePath,
        m_Level.levelWidth,
        m_Level.levelHeight,
        m_Level.playerSpawn
    );

    // ── 依關卡主題決定背景音樂（載入時決定，EnterPlaying 時才真正播放）──
    m_LevelBGMName = (m_Level.theme == "underground") ? "underground" : "overworld";

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
            auto b = std::make_shared<BrickBlock>(pos, m_ThemeAssets);
            b->SetItemType(obj.itemType.empty() ? "None" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "UsedOnHitBrickBlock") {
            auto b = std::make_shared<UsedOnHitBrickBlock>(pos, m_ThemeAssets);
            b->SetItemType(obj.itemType.empty() ? "None" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "QuestionBlock") {
            auto b = std::make_shared<QuestionBlock>(pos, m_ThemeAssets);
            b->SetItemType(obj.itemType.empty() ? "Coin" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "HiddenBlock" || obj.type == "HiddenQuestionBlock") {
            auto b = std::make_shared<HiddenBlock>(pos, m_ThemeAssets);
            b->SetItemType(obj.itemType.empty() ? "Coin" : obj.itemType);
            m_Blocks.push_back(b);
        } else if (obj.type == "MultiCoinBlock") {
            m_Blocks.push_back(std::make_shared<MultiCoinBlock>(pos, m_ThemeAssets, obj.coinCount));
        } else if (obj.type == "Pipe" || obj.type == "EnterablePipe") {
            const glm::vec2 pipeSize = GetPipeSize(obj.opening, obj.segments);
            const glm::vec2 pipePosition = GetPipePositionFromAnchor(pos, obj.opening, obj.segments);
            m_Blocks.push_back(std::make_shared<EnterablePipeBlock>(
                pipePosition,
                pipeSize,
                obj.opening,
                obj.targetLevel,
                obj.exitToLevel,
                obj.targetSpawn,
                obj.hasTargetSpawn));
        } else if (obj.type == "PipeCollision") {
            m_Blocks.push_back(std::make_shared<PipeCollisionBlock>(pos, size));
        } else if (obj.type == "MovingPlatform") {
            const glm::vec2 platformSize = {
                TILE_SIZE * static_cast<float>(std::max(1, obj.segments)),
                TILE_SIZE * 0.5f
            };
            const float moveDistance = static_cast<float>(std::max(0, obj.moveTiles)) * TILE_SIZE;
            m_Blocks.push_back(std::make_shared<MovingPlatformBlock>(
                pos,
                platformSize,
                obj.moveAxis,
                moveDistance,
                obj.moveSpeed,
                obj.moveMode,
                obj.startDirection));
        } else if (obj.type == "TreePlatform") {
            m_Blocks.push_back(std::make_shared<TreePlatformBlock>(pos, obj.segments));
        } else if (obj.type == "Wall") {
            m_Blocks.push_back(std::make_shared<WallBlock>(pos, m_ThemeAssets));
        } else if (obj.type == "Flag") {
            m_Blocks.push_back(std::make_shared<FlagBlock>(pos));
        } else if (obj.type == "Coin" || obj.type == "CollectibleCoin") {
            m_Items.push_back(std::make_shared<LevelCoinItem>(pos, m_ThemeAssets));
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

    return std::string(RESOURCE_DIR) + "/data/" +
           levelName + (hasJsonSuffix ? "" : ".json");
}

void GameManager::ChangeLevel(const std::string& levelName, std::optional<glm::vec2> spawnOverride) {
    const std::string levelPath = MakeLevelPath(levelName);
    if (levelPath.empty()) return;

    LOG_INFO("Changing level to '{}'", levelPath);

    ResetSceneObjects();
    m_LevelCleared = false;

    // 換關（水管切換關卡）：清空中繼點，新關卡從頭開始
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;

    LoadLevel(levelPath);
    if (spawnOverride.has_value()) {
        m_Player.SetSpawnPosition(*spawnOverride);
        LOG_INFO("Player spawn override applied: {}", m_Player.GetPosition());
    }

    {
        const float lvW = static_cast<float>(m_Level.levelWidth);
        m_Camera.SetX(std::clamp(
            m_Player.GetPosition().x - m_Camera.GetViewWorldWidth() * 0.5f,
            0.0f, std::max(0.0f, lvW - m_Camera.GetViewWorldWidth())));
    }
    BuildScene();
    m_Hud.Init(m_Renderer, m_SelectedWorldLabel);

    // ─── 檢查是否起點是個水管，如果是則觸發鑽出動畫 ───
    glm::vec2 pPos = m_Player.GetPosition();
    glm::vec2 pSize = m_Player.GetSize();
    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;
        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe || !pipe->IsEnterable()) continue;

        glm::vec2 bPos = pipe->GetPosition();
        glm::vec2 bSize = pipe->GetSize();
        
        bool insideX = (pPos.x + pSize.x * 0.5f) >= bPos.x && (pPos.x + pSize.x * 0.5f) <= (bPos.x + bSize.x);
        bool insideY = (pPos.y + pSize.y * 0.5f) >= bPos.y && (pPos.y + pSize.y * 0.5f) <= (bPos.y + bSize.y);
        if (insideX && insideY) {
            m_Player.StartPipeExit(bPos, bSize, pipe->GetOpening(), 1.0f);
            break;
        }
    }
}

bool GameManager::CheckPipeTransition() {
    const bool pressingDown =
        Util::Input::IsKeyPressed(Util::Keycode::DOWN) ||
        Util::Input::IsKeyPressed(Util::Keycode::S);
    const bool pressingUp =
        Util::Input::IsKeyPressed(Util::Keycode::UP) ||
        Util::Input::IsKeyPressed(Util::Keycode::W);
    const bool pressingLeft =
        Util::Input::IsKeyPressed(Util::Keycode::LEFT) ||
        Util::Input::IsKeyPressed(Util::Keycode::A);
    const bool pressingRight =
        Util::Input::IsKeyPressed(Util::Keycode::RIGHT) ||
        Util::Input::IsKeyPressed(Util::Keycode::D);

    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;

        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe) continue;
        if (!pipe->CanEnter(m_Player, pressingUp, pressingDown, pressingLeft, pressingRight)) continue;

        const std::string targetLevel = pipe->GetDestinationLevel();
        if (targetLevel.empty()) {
            LOG_WARN("Enterable pipe at {} has no target level.", pipe->GetPosition());
            return false;
        }

        m_PendingLevel = targetLevel;
        m_PendingSpawn = pipe->GetDestinationSpawn();
        m_Player.StartPipeEntry(pipe->GetPosition(), pipe->GetSize(), pipe->GetOpening(), 1.0f);
        m_Audio.PlaySFX("pipe"); // 進水管音效
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
    constexpr float SPAWN_BUFFER = 32.0f;
    const float cameraRight = m_Camera.GetX() + m_Camera.GetViewWorldWidth() + SPAWN_BUFFER;

    // 效能優化：用 partition 取代每幀配置 remaining 向量。
    // stable_partition 把「還沒到」的留在前半，「已到」的移到後半，再 spawn + erase。
    auto spawnBegin = std::stable_partition(
        m_EnemySpawnQueue.begin(), m_EnemySpawnQueue.end(),
        [&](const ObjectData& d) { return d.x > cameraRight; });

    for (auto it = spawnBegin; it != m_EnemySpawnQueue.end(); ++it) {
        SpawnEnemy(*it);
    }
    m_EnemySpawnQueue.erase(spawnBegin, m_EnemySpawnQueue.end());
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

    if      (data.enemyType == "Goomba")       newEnemy = std::make_shared<Goomba>(data.x, data.y, m_ThemeAssets);
    else if (data.enemyType == "Koopa")        newEnemy = std::make_shared<Koopa>(data.x, data.y, koopaVariant, m_ThemeAssets);
    else if (data.enemyType == "KoopaParatroopa") {
        newEnemy = std::make_shared<KoopaParatroopa>(data.x, data.y, koopaVariant, flightMode, m_ThemeAssets);
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

    // 玩家落地時重置踩踏連殺 combo
    if (m_Player.IsOnGround()) {
        ResetCombo();
    }

    for (auto& enemy : m_Enemies) {
        if (!enemy->IsAlive()) continue;

        const glm::vec2 ePos  = enemy->GetPosition();
        const glm::vec2 eSize = enemy->GetSize();

        // 矩形重疊判斷（AABB）
        if (!CollisionUtils::CheckAABB(pPos, pSize, ePos, eSize)) continue;

        if (m_Player.IsStarInvincible()) {
            // 星星無敵：觸發翻轉死亡（使用 combo 計分）
            auto* koopa = dynamic_cast<Koopa*>(enemy.get());
            if (koopa) {
                const bool flipLeft = (enemy->GetPosition().x + enemy->GetSize().x * 0.5f) >=
                                      (m_Player.GetPosition().x + m_Player.GetSize().x * 0.5f);
                koopa->Die(flipLeft);
            } else {
                enemy->SetAlive(false);
                enemy->SetVisible(false);
            }
            {
                const int pts = NextComboScore();
                if (pts < 0) {
                    m_Session.AddLife();
                    SpawnScorePopup(-1, ePos);
                } else {
                    m_Session.AddScore(pts);
                    SpawnScorePopup(pts, ePos);
                }
            }
            LOG_INFO("Star invincibility defeated enemy.");
            continue;
        }

        // ── 判斷是否為壓扁中的 Goomba（不具傷害性）──
        auto* goomba = dynamic_cast<Goomba*>(enemy.get());
        if (goomba && goomba->IsSquashed()) continue;

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
            if (isStationaryShell) {
                // 從上方踩到縮殼中的龜 → 踢殼！（踢殼算 400 分，不進 combo）
                const bool playerIsLeftOfShell =
                    (pPos.x + pSize.x * 0.5f) < (ePos.x + eSize.x * 0.5f);
                const bool kickLeft = !playerIsLeftOfShell;
                koopa->Kick(kickLeft);
                m_Session.AddScore(400);
                SpawnScorePopup(400, ePos);
                m_Audio.PlaySFX("kick"); // 踢殼音效
                LOG_INFO("Koopa shell kicked {} from top!", kickLeft ? "left" : "right");
            } else {
                // 一般踩踏（包含首次踩到正常行走的龜）— combo 計分
                enemy->Stomp();
                const int pts = NextComboScore();
                if (pts < 0) {
                    m_Session.AddLife();
                    SpawnScorePopup(-1, ePos);
                    m_Audio.PlaySFX("1up"); // 踩踏 1UP 音效
                } else {
                    m_Session.AddScore(pts);
                    SpawnScorePopup(pts, ePos);
                }
                m_Audio.PlaySFX("stomp"); // 踩踏音效
                LOG_INFO("Stomp! combo={} score={}", m_ComboCount, pts);
            }

            // 踩踏後給玩家一個小彈跳
            glm::vec2 vel = m_Player.GetVelocity();
            vel.y = -180.0f;
            m_Player.SetVelocity(vel);
        } else {
            // ── 側碰 / 從下方衝入 ──
            if (isStationaryShell) {
                // 從側面碰到靜止的龜殼 → 直接踢飛（400 分），且不會有往上的彈跳！
                const bool playerIsLeftOfShell =
                    (pPos.x + pSize.x * 0.5f) < (ePos.x + eSize.x * 0.5f);
                const bool kickLeft = !playerIsLeftOfShell;
                koopa->Kick(kickLeft);
                m_Session.AddScore(400);
                SpawnScorePopup(400, ePos);
                m_Audio.PlaySFX("kick"); // 踢殼音效
                LOG_INFO("Koopa shell kicked {} from side!", kickLeft ? "left" : "right");
            } else {
                // 碰到有殺傷力的敵人：玩家受傷降級
                // 變身動畫中視為無敵，不可被重複傷害
                if (!m_Player.IsDamageInvincible() && !m_Player.IsTransforming()) {
                    m_Player.Downgrade();
                    if (m_Player.IsDying()) {
                        // SMALL 直接死亡，無無敵倒數需求
                    } else if (m_Player.IsTransforming()) {
                        // 觸發縮小動畫：無敵倒數等動畫結束後才開始
                        m_PendingDamageInvincibility = 2.0f;
                    }
                    LOG_INFO("Player hit by enemy! Downgrade triggered.");
                }
            }
        }
    }
}


// ─── CheckShellEnemyCollision ─────────────────────────────────────────────
// 掃描所有正在滑行的 Koopa 殼，判斷是否與其他存活的敵人重疊。
// 重疊則直接消滅被撞到的敵人，並使用獨立的殼連殺 combo 計分。
void GameManager::CheckShellEnemyCollision() {
    bool shellKilledAny = false;  // 追蹤本幀是否有殼殺敵

    for (auto& shellEnemy : m_Enemies) {
        if (!shellEnemy->IsAlive()) continue;

        auto* koopa = dynamic_cast<Koopa*>(shellEnemy.get());
        if (!koopa || !koopa->IsSliding()) continue;  // 只有滑行中的殼才能傷到敵人

        const glm::vec2 sPos  = koopa->GetPosition();
        const glm::vec2 sSize = koopa->GetSize();

        for (auto& target : m_Enemies) {
            if (target.get() == shellEnemy.get()) continue;  // 不撞自己
            if (!target->IsAlive()) continue;

            const glm::vec2 tPos  = target->GetPosition();
            const glm::vec2 tSize = target->GetSize();

            if (CollisionUtils::CheckAABB(sPos, sSize, tPos, tSize)) {
                // 殼命中 Koopa 系列：觸發翻轉死亡
                auto* targetKoopa = dynamic_cast<Koopa*>(target.get());
                if (targetKoopa) {
                    const bool flipLeft = koopa->GetVelocity().x < 0.0f;
                    targetKoopa->Die(flipLeft);
                } else {
                    target->SetAlive(false);
                    target->SetVisible(false);
                }
                // 殼連殺使用獨立的殼 combo 計分
                const int pts = NextShellComboScore();
                if (pts < 0) {
                    m_Session.AddLife();
                    SpawnScorePopup(-1, tPos);
                } else {
                    m_Session.AddScore(pts);
                    SpawnScorePopup(pts, tPos);
                }
                LOG_INFO("Shell killed enemy: combo={} score={} shellPos={} targetPos={}",
                         m_ShellComboCount, pts, sPos, tPos);
                shellKilledAny = true;
            }
        }
    }

    // 如果本幀沒有殼殺到任何敵人，重置殼 combo
    if (!shellKilledAny) {
        m_ShellComboCount = 0;
    }
}

// ─── CheckBlockCollision ──────────────────────────────────────────────────
void GameManager::CheckBlockCollision() {
    // ── Bug 1 修復：分兩個 pass 處理 ──────────────────────────────────────
    // 問題根源：原本在同一迴圈中邊掃描邊修改 pPos，
    // 導致後面的方塊用「已位移後的位置」做 hitFromBelow 判斷，
    // 造成站在 B 正下方往上跳卻觸發 A 或 C 的 OnHit。
    //
    // 修復策略：
    //   Pass 1：用原始 pPos/pPrev 找出所有「從下方碰撞」的候選方塊，
    //            取 blockBottom 最大的那個（最靠近頭頂），只觸發它的 OnHit。
    //   Pass 2：對所有重疊方塊做位置修正（落地、側碰）。
    // ──────────────────────────────────────────────────────────────────────

    const glm::vec2 pPosOrig = m_Player.GetPosition();  // 原始位置，整個函式內不變
    const glm::vec2 pPrev    = m_Player.GetPreviousPosition();
    const glm::vec2 pSize    = m_Player.GetSize();
    glm::vec2 pPos           = pPosOrig;                // 工作用位置，可累積修正
    glm::vec2 pVel           = m_Player.GetVelocity();
    bool onGround = false;

    // 效能優化：複用 member 暫存容器，避免每幀 heap allocation
    m_TmpBlocksToRemove.clear();

    constexpr float SURFACE_TOLERANCE = 2.0f;

    // ── Pass 1：找到唯一的 hitFromBelow 候選 ─────────────────────────────
    // 選擇標準：水平重疊量最大的方塊 = 馬力歐「最正對」的那格。
    // 不能用 blockBottom 最大：同一排 ABCD 的 blockBottom 完全相同，
    // 用 blockBottom 只會選到迴圈最後通過的那個，導致打到旁邊的方塊。
    std::shared_ptr<Block> hitBelowBlock = nullptr;
    float bestOverlapX = -1.0f;

    if (pVel.y < 0.0f) { // 只有往上飛時才可能從下方碰
        const float previousTop = pPrev.y;

        for (const auto& block : m_Blocks) {
            // HiddenBlock 揭露前 IsSolid() == false，但往上頂碰時仍需觸發 OnHit（揭露效果）。
            // 因此對未揭露的 HiddenBlock 也要進入 Pass 1 掃描；Pass 2 因 IsSolid() == false
            // 會跳過它，所以揭露前不會阻擋玩家走路或站立。
            const auto* hiddenBlk = dynamic_cast<const HiddenBlock*>(block.get());
            const bool isUnrevealedHidden = hiddenBlk && !hiddenBlk->IsRevealed();
            if (!block->IsSolid() && !isUnrevealedHidden) continue;

            const glm::vec2 bPos  = block->GetPosition();
            const glm::vec2 bSize = block->GetSize();

            // 用原始 pPosOrig 判斷是否重疊
            if (!CollisionUtils::CheckAABB(pPosOrig, pSize, bPos, bSize)) continue;

            const float blockBottom = bPos.y + bSize.y;

            const bool hitFromBelow =
                previousTop >= blockBottom - SURFACE_TOLERANCE &&
                pPosOrig.y <= blockBottom;

            if (hitFromBelow) {
                // 計算水平重疊量，選最大的（即最正對的方塊）
                const float overlapLeft  = std::max(pPosOrig.x, bPos.x);
                const float overlapRight = std::min(pPosOrig.x + pSize.x, bPos.x + bSize.x);
                const float overlapX = std::max(0.0f, overlapRight - overlapLeft);

                if (overlapX > bestOverlapX) {
                    bestOverlapX = overlapX;
                    hitBelowBlock = block;
                }
            }
        }
    }


    // 若找到 hitFromBelow 方塊，先處理它（彈開玩家並呼叫 OnHit）
    if (hitBelowBlock) {
        const glm::vec2 bPos  = hitBelowBlock->GetPosition();
        const glm::vec2 bSize = hitBelowBlock->GetSize();

        const float bCenterY  = bPos.y + bSize.y * 0.5f;
        const float pCenterY  = pPosOrig.y + pSize.y * 0.5f;
        const float penY      = (pSize.y * 0.5f + bSize.y * 0.5f) - std::abs(pCenterY - bCenterY);

        pPos.y += penY;
        pVel.y  = 0.0f;

        BlockHitResult hitRes = hitBelowBlock->OnHit(&m_Player);

        if (hitRes.isDestroyed) {
            SpawnBrickDebris(bPos);
            hitBelowBlock->SetVisible(false);
            m_TmpBlocksToRemove.push_back(hitBelowBlock);
            // 磚塊打破得 50 分
            m_Session.AddScore(50);
            SpawnScorePopup(50, bPos);
            m_Audio.PlaySFX("brick_break"); // 磚塊破碎音效
        }

        if (hitRes.spawnItem != "None") {
            SpawnItem(hitRes.spawnItem, bPos);
        } else if (!hitRes.isDestroyed) {
            // 頂到方塊但沒有打破也沒有道具：播 bump 音效（頂空磚或不可破磚）
            m_Audio.PlaySFX("bump");
        }

        // ── 頂磚效果：消滅方塊正上方的敵人，踢飛正上方的道具 ──────────
        // 判定範圍：敵人/道具底部距方塊頂部 ±BUMP_ABOVE_TOL 像素內，
        // 且水平中心在方塊左右邊界以內（含半格緩衝）。
        constexpr float BUMP_ABOVE_TOL  = 4.0f;   // 距方塊頂部的垂直容忍
        constexpr float BUMP_ITEM_KICK  = -200.0f; // 道具被踢飛的上拋初速

        const float blockTopY   = bPos.y;
        const float blockLeft   = bPos.x;
        const float blockRight  = bPos.x + bSize.x;

        for (auto& enemy : m_Enemies) {
            if (!enemy->IsAlive()) continue;

            const glm::vec2 ePos  = enemy->GetPosition();
            const glm::vec2 eSize = enemy->GetSize();

            const float enemyBottom  = ePos.y + eSize.y;
            const float enemyCenterX = ePos.x + eSize.x * 0.5f;

            // 底部接近方塊頂面，且水平有重疊
            if (std::abs(enemyBottom - blockTopY) <= BUMP_ABOVE_TOL &&
                enemyCenterX >= blockLeft && enemyCenterX <= blockRight) {
                // 踩踏式消滅（效果等同被從上方踩）
                enemy->Stomp();
                LOG_INFO("Bump-block killed enemy above block at ({}, {})", bPos.x, bPos.y);
            }
        }

        for (auto& item : m_Items) {
            if (item->GetState() != ItemState::Active) continue;

            const glm::vec2 iPos  = item->GetPosition();
            const glm::vec2 iSize = item->GetSize();

            const float itemBottom  = iPos.y + iSize.y;
            const float itemCenterX = iPos.x + iSize.x * 0.5f;

            if (std::abs(itemBottom - blockTopY) <= BUMP_ABOVE_TOL &&
                itemCenterX >= blockLeft && itemCenterX <= blockRight) {
                // 給道具一個向上的速度踢飛
                glm::vec2 iVel = item->GetVelocity();
                iVel.y = BUMP_ITEM_KICK;
                item->SetVelocity(iVel);
                LOG_INFO("Bump-block kicked item above block at ({}, {})", bPos.x, bPos.y);
            }
        }
    }

    // ── Pass 2：處理落地與側碰（使用累積修正後的 pPos）─────────────────
    for (const auto& block : m_Blocks) {
        if (!block->IsSolid()) continue;
        if (block == hitBelowBlock) continue; // 已在 Pass 1 處理

        const glm::vec2 bPos  = block->GetPosition();
        const glm::vec2 bSize = block->GetSize();

        if (!CollisionUtils::CheckAABB(pPos, pSize, bPos, bSize)) continue;

        const float pCenterX = pPos.x + pSize.x * 0.5f;
        const float pCenterY = pPos.y + pSize.y * 0.5f;
        const float bCenterX = bPos.x + bSize.x * 0.5f;
        const float bCenterY = bPos.y + bSize.y * 0.5f;

        const float dx   = pCenterX - bCenterX;
        const float dy   = pCenterY - bCenterY;
        const float penX = (pSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
        const float penY = (pSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

        const float previousBottom = pPrev.y + pSize.y;
        const float currentBottom  = pPos.y + pSize.y;
        const float overlapLeft    = std::max(pPos.x, bPos.x);
        const float overlapRight   = std::min(pPos.x + pSize.x, bPos.x + bSize.x);
        const float overlapX       = std::max(0.0f, overlapRight - overlapLeft);
        const float blockTop       = bPos.y;

        // 修復（快速下落著地）：讓容忍值隨下落速度自適應。
        // 高速下落時玩家一幀穿過的距離可能超過固定的 SURFACE_TOLERANCE，
        // 改用 max(固定值, |vy|*dt + 小緩衝) 確保都能被正確接住。
        // dt 固定用 MAX_FRAME_DT 的上界防止超大值；+2 是最小緩衝。
        const float adaptiveTol = std::max(SURFACE_TOLERANCE,
                                           std::abs(pVel.y) * MAX_FRAME_DT + 2.0f);
        const bool landedFromAbove =
            pVel.y >= 0.0f &&
            previousBottom <= blockTop + adaptiveTol &&
            currentBottom >= blockTop;

        // Bug 2 修復：standingOverlap 加入 penY < penX 條件，
        // 確保 Y 軸穿透比 X 軸小才視為從上方著地，
        // 避免快速水平移動側碰地板被誤判為落地而「被拉上去」。
        // 修復（擴充）：原本只涵蓋 Ground，現在擴充到所有 solid 方塊，
        // 確保玩家站在磚塊、問號磚、牆壁等方塊頂部也能正確落地。
        const bool standingOverlap =
            pVel.y >= 0.0f &&
            dy < 0.0f &&
            penY < penX &&
            penY <= TILE_SIZE + 4.0f &&
            overlapX >= pSize.x * 0.5f;

        if (landedFromAbove || standingOverlap) {
            pPos.y -= penY;
            if (pVel.y > 0.0f) pVel.y = 0.0f;
            onGround = true;
            if (auto* platform = dynamic_cast<MovingPlatformBlock*>(block.get())) {
                pPos += platform->GetFrameDelta();
            }
        } else {
            // 側碰（hitFromBelow 已在 Pass 1 排除）
            if (dx > 0.0f) pPos.x += penX;
            else           pPos.x -= penX;
        }
    }

    m_Player.SetPosition(pPos);
    m_Player.SetVelocity(pVel);
    m_Player.SetOnGround(onGround);

    // 清理碎裂的方塊（使用 member 暫存容器）
    for (auto& b : m_TmpBlocksToRemove) {
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
    std::string spawnedType;

    if (itemType == "Coin") {
        newItem = std::make_shared<CoinItem>(position, m_ThemeAssets);
        spawnedType = "Coin";
        m_Session.AddCoin();
        m_Session.AddScore(200);
        SpawnScorePopup(200, position);
        m_Audio.PlaySFX("coin"); // 金幣音效
    } else if (itemType == "PowerUp" || itemType == "Mushroom") {
        if (m_Player.GetForm() == Player::Form::SMALL) {
            newItem = std::make_shared<MushroomItem>(position, m_ThemeAssets);
            spawnedType = "Mushroom";
        } else {
            newItem = std::make_shared<FireFlowerItem>(position, m_ThemeAssets);
            spawnedType = "FireFlower";
        }
        m_Audio.PlaySFX("powerup_appears"); // 道具冒出音效
    } else if (itemType == "FireFlower") {
        newItem = std::make_shared<FireFlowerItem>(position, m_ThemeAssets);
        spawnedType = "FireFlower";
        m_Audio.PlaySFX("powerup_appears"); // 道具冒出音效
    } else if (itemType == "OneUp" || itemType == "1Up") {
        newItem = std::make_shared<OneUpMushroomItem>(position, m_ThemeAssets);
        spawnedType = "OneUp";
        m_Audio.PlaySFX("powerup_appears"); // 道具冒出音效
    } else if (itemType == "Star" || itemType == "Starman") {
        newItem = std::make_shared<StarmanItem>(position, m_ThemeAssets);
        spawnedType = "Star";
        m_Audio.PlaySFX("powerup_appears"); // 道具冒出音效
    }

    if (newItem) {
        m_Items.push_back(newItem);
        m_Renderer.AddChild(newItem);
        LOG_INFO("Item spawned: requested='{}' actual='{}' position={}",
                 itemType,
                 spawnedType,
                 position);
    } else {
        LOG_WARN("Unknown item type requested: '{}' at {}", itemType, position);
    }
}

// ─── CheckItemCollision ───────────────────────────────────────────────────
void GameManager::CheckItemCollision() {
    const glm::vec2 pPos  = m_Player.GetPosition();
    const glm::vec2 pSize = m_Player.GetSize();

    // 效能優化：複用 member 暫存容器
    m_TmpItemsToRemove.clear();

    for (auto& item : m_Items) {
        // 香菇只有在 Active 狀態才能吃
        // 金幣不透過接觸吃，但金幣生命週期結束變成 Collected 也會移除
        if (item->GetState() == ItemState::Active && item->GetType() != "Coin") {
            const glm::vec2 iPos  = item->GetPosition();
            const glm::vec2 iSize = item->GetSize();

            if (CollisionUtils::CheckAABB(pPos, pSize, iPos, iSize)) {
                const std::string itemType = item->GetType();
                item->OnCollect(&m_Player);
                if (item->GetState() == ItemState::Collected) {
                    LOG_INFO("Item collected: type='{}' position={}", itemType, iPos);
                    if (itemType == "LevelCoin") {
                        m_Session.AddCoin();
                        m_Session.AddScore(200);
                        SpawnScorePopup(200, iPos);
                        m_Audio.PlaySFX("coin"); // 場上金幣音效
                    } else if (itemType == "OneUp") {
                        m_Session.AddLife();
                        SpawnScorePopup(-1, iPos); // -1 = "1UP"
                        m_Audio.PlaySFX("1up");   // 1UP 音效
                    } else if (itemType == "Mushroom" || itemType == "FireFlower") {
                        m_Session.AddScore(1000);
                        SpawnScorePopup(1000, iPos);
                        m_Audio.PlaySFX("powerup"); // 吃到升級道具音效
                    } else if (itemType == "Star") {
                        m_Session.AddScore(1000);
                        SpawnScorePopup(1000, iPos);
                        m_Audio.PlaySFX("powerup");  // 吃到道具音效
                        // 吃星後切換為 Starman BGM（星星時間結束後恢復，在 UpdatePlaying 偵測）
                        m_Audio.PlayBGM("starman");
                    }
                }
            }
        }

        // 把狀態變為 Collected 的項目加入移除名單
        if (item->GetState() == ItemState::Collected) {
            item->SetVisible(false);
            m_TmpItemsToRemove.push_back(item);
        }
    }

    for (auto& i : m_TmpItemsToRemove) {
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
    m_Audio.PlaySFX("fireball"); // 火球發射音效
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
        auto debris = std::make_shared<BrickDebris>(center + offsets[i], velocities[i], m_ThemeAssets);
        m_BrickDebris.push_back(debris);
        m_Renderer.AddChild(debris);
    }
}

// ─── CheckFireballCollision ───────────────────────────────────────────────
void GameManager::CheckFireballCollision() {
    // 效能優化：複用 member 暫存容器
    m_TmpFireballsToRemove.clear();
    const float killZ = m_Level.levelHeight + 50.0f;

    for (auto& fireball : m_Fireballs) {
        if (fireball->IsDead()) {
            m_TmpFireballsToRemove.push_back(fireball);
            continue;
        }

        // 當火球正在播放碎片爆炸動畫時，不再計算碰撞
        if (fireball->IsExploded()) continue;

        glm::vec2 fPos  = fireball->GetPosition();
        glm::vec2 fSize = fireball->GetSize();
        glm::vec2 fVel  = fireball->GetVelocity();
        bool collidedThisFrame = false;

        // 1. 與方塊的碰撞
        for (const auto& block : m_Blocks) {
            if (!block->IsSolid()) continue;

            const glm::vec2 bPos  = block->GetPosition();
            const glm::vec2 bSize = block->GetSize();

            if (!CollisionUtils::CheckAABB(fPos, fSize, bPos, bSize)) continue;

            const float dx = (fPos.x + fSize.x * 0.5f) - (bPos.x + bSize.x * 0.5f);
            const float dy = (fPos.y + fSize.y * 0.5f) - (bPos.y + bSize.y * 0.5f);
            const float penX = (fSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
            const float penY = (fSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);

            if (penX < penY) {
                fireball->Explode();
                collidedThisFrame = true;
                break;
            }

            if (dy < 0.0f && fVel.y > 0.0f) {
                fPos.y -= penY;
                fireball->SetPosition(fPos);
                fireball->Bounce();
                fVel = fireball->GetVelocity();
            } else {
                fireball->Explode();
                collidedThisFrame = true;
                break;
            }
        }

        if (collidedThisFrame) continue;

        // 2. 與敵人的碰撞
        for (auto& enemy : m_Enemies) {
            if (!enemy->IsAlive()) continue;

            // 食人花隱藏中不受火球傷害（由 PiranhaPlant::IsHidden() 保護）
            auto* piranha = dynamic_cast<PiranhaPlant*>(enemy.get());
            if (piranha && piranha->IsHidden()) continue;

            if (CollisionUtils::CheckAABB(fPos, fSize, enemy->GetPosition(), enemy->GetSize())) {
                const glm::vec2 ePos = enemy->GetPosition();
                // Koopa 系列：觸發翻轉死亡，得 200 分；其他敵人（Goomba 等）得 100 分
                auto* koopa = dynamic_cast<Koopa*>(enemy.get());
                const bool isKoopa = (koopa != nullptr);
                if (koopa) {
                    const bool flipLeft = fVel.x < 0.0f; // 火球飛行方向決定翻轉朝向
                    koopa->Die(flipLeft);
                } else {
                    enemy->SetAlive(false);
                    enemy->SetVisible(false);
                }
                const int pts = isKoopa ? 200 : 100;
                m_Session.AddScore(pts);
                SpawnScorePopup(pts, ePos);
                fireball->Explode();
                LOG_INFO("Fireball defeated enemy: score={} fireballPos={} enemyPos={}",
                         pts, fPos, ePos);
                break;
            }
        }

        // 3. 虛空判定
        if (fPos.y > killZ) fireball->Explode();
    }

    // 移除死掉的火球
    for (auto& fb : m_TmpFireballsToRemove) {
        m_Fireballs.erase(std::remove(m_Fireballs.begin(), m_Fireballs.end(), fb), m_Fireballs.end());
    }
}

// ─── UpdateCheckpoints ────────────────────────────────────────────────────
// 每幀呼叫：掃描關卡中繼點清單，若玩家 X 越過某中繼點 X，
// 且比目前已記錄的中繼點更靠右，就更新 m_LastCheckpoint。
void GameManager::UpdateCheckpoints() {
    if (m_Level.checkpoints.empty()) return;

    const float playerX = m_Player.GetPosition().x;

    for (const auto& cp : m_Level.checkpoints) {
        // 玩家中心 X 越過中繼點 X 才算達成
        if (playerX >= cp.x) {
            if (!m_LastCheckpoint.has_value() ||
                cp.x > m_LastCheckpoint->x) {
                m_LastCheckpoint = cp;
                LOG_INFO("Checkpoint reached: x={} y={}", cp.x, cp.y);
            }
        }
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

        // 觸發旗球下降動畫（與玩家下滑同時進行）
        flagBlock->StartDescent();

        // 觸發玩家的過關下降演出
        m_Player.StartLevelClearSequence(fPos.x, fPos.y + fSize.y);

        // 碰到旗杆：播放旗杆 SFX，同時停掉關卡 BGM
        m_Audio.StopBGM();
        m_Audio.PlaySFX("flagpole");
        LOG_INFO("=== LEVEL CLEAR! Flag score: {}  Total score: {} ===",
                 score,
                 m_Session.CurrentPlayer().score);
        break;
    }
}
