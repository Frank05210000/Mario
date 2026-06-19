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
#include <cmath>
#include <iomanip>
#include <limits>
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

// ─── 關卡鏈定義 ───────────────────────────────────────────────────────────
// 過關順序：1-1 → 1-2 → 1-3 → 回標題
// 新增關卡只需在此陣列末尾追加元素
const std::vector<GameManager::LevelEntry> GameManager::kLevelChain = {
    {"1-1_ground_1", "1-1"},
    {"1-2_ground_1", "1-2"},
    {"1-3_ground_1", "1-3"},
};

// ─── Combo 分數序列（NES Mario 踩踏 / 殼連殺）────────────────────────────
// 第 n 次連殺的分值：100, 200, 400, 500, 800, 1000, 2000, 4000, 5000, 1UP...
static const std::array<int, 9> kComboScores = {
    100, 200, 400, 500, 800, 1000, 2000, 4000, 5000
};

static constexpr int kParatroopaDemoteScore = 400;
static constexpr float kCheckpointEnemySafeRadius = TILE_SIZE * 3.0f;

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

static int ShellComboScore(int idx) {
    if (idx < static_cast<int>(kComboScores.size())) {
        return kComboScores[idx];
    }
    return -1;
}

static std::string FormatScore6(int score) {
    std::ostringstream ss;
    ss << std::setw(6) << std::setfill('0') << std::min(score, 999999);
    return ss.str();
}

void GameManager::ResetCombo() {
    m_ComboCount = 0;
}

// ─── 浮動得分彈出 ─────────────────────────────────────────────────────────
void GameManager::SpawnScorePopup(int score, glm::vec2 worldPos) {
    const std::string fontPath = MakeAssetPath("font/Super Mario Bros. NES.ttf");
    std::string label = (score < 0) ? "1UP" : std::to_string(score);

    auto text = std::make_shared<Util::Text>(
        fontPath, 18, label, Util::Color(255, 255, 255));

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

// ─── 過關城堡升旗 ───────────────────────────────────────────────────────
//
// 背景圖已畫死城堡本體（含城垛），這裡只負責「會動的小旗」。
// 原版看起來像小旗從城堡頂冒出；由於背景是單張不透明圖，不能讓 sprite
// 真正躲在城牆後，所以用底部固定、可見高度往上長的方式模擬。
//
// doorCenterX：城堡門中心 X（由 Player 依 Flag.clearWalkTiles 回傳）
// groundY    ：地面 Y（= 碰旗時的旗杆底 Y）
void GameManager::SpawnCastleFlag(float doorCenterX, float groundY, float castleFlagBaseTiles) {
    constexpr float CASTLE_FLAG_X_OFFSET_TILES = 1.0f;

    // tutorial 之類：城堡會超出關卡右界 → 沒有城堡可升旗，直接略過
    if (doorCenterX + TILE_SIZE > static_cast<float>(m_Level.levelWidth)) {
        LOG_INFO("Castle flag skipped (no castle within level bounds). doorX={}", doorCenterX);
        return;
    }

    auto image = std::make_shared<Util::Image>(MakeAssetPath("item/flag/castle_flag.png"));
    m_CastleFlag = std::make_shared<Util::GameObject>(image, 9.0f); // 背景之上、HUD 之下

    const auto textureSize = image->GetSize();
    m_CastleFlagFullHeight = std::max(1.0f, textureSize.y);
    m_CastleFlagVisibleHeight = 0.0f;
    m_CastleFlagBaseY = groundY - castleFlagBaseTiles * TILE_SIZE;
    m_CastleFlagWorldPos = {doorCenterX + CASTLE_FLAG_X_OFFSET_TILES * TILE_SIZE, m_CastleFlagBaseY};

    UpdateCastleFlagRaise(0.0f);
    m_CastleFlag->SetVisible(true);
    m_Renderer.AddChild(m_CastleFlag);

    LOG_INFO("Castle flag raise started. doorX={} baseY={} fullHeight={}",
             doorCenterX, m_CastleFlagBaseY, m_CastleFlagFullHeight);
}

void GameManager::UpdateCastleFlagRaise(float dt) {
    if (!m_CastleFlag) return;
    if (m_CastleFlagVisibleHeight < m_CastleFlagFullHeight) {
        m_CastleFlagVisibleHeight += CASTLE_FLAG_SPEED * dt;
        if (m_CastleFlagVisibleHeight > m_CastleFlagFullHeight) {
            m_CastleFlagVisibleHeight = m_CastleFlagFullHeight;
        }
    }

    const float scaleRatio = m_CastleFlagVisibleHeight / m_CastleFlagFullHeight;
    m_CastleFlag->m_Transform.scale = {GAME_SCALE, GAME_SCALE * scaleRatio};
    m_CastleFlagWorldPos.y = m_CastleFlagBaseY - m_CastleFlagVisibleHeight * 0.5f;
    m_CastleFlag->m_Transform.translation = m_Camera.WorldToScreen(m_CastleFlagWorldPos);
}

namespace {
constexpr float kTitleTextZ = 30.0f;
constexpr float kDeathSequenceMinDuration = 3.9f; // death.mp3 is about 3.87s
constexpr float kPipeTransitionDuration = 2.0f;   // pipe.mp3 is 2.00s
constexpr float kGameOverDuration = 4.8f;          // game_over.mp3 is about 4.72s
constexpr float kLevelClearBgmDuration = 6.5f;     // level_clear.mp3 is about 6.49s
constexpr const char* kStarmanBGMName = "starman";
constexpr const char* kDebugStarmanBGMName = "debug_starman_hurry_loop";

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

bool ShouldStartPipeExitForSpawn(const PipeBlock& pipe,
                                 glm::vec2 playerPos,
                                 glm::vec2 playerSize) {
    constexpr float kPipeExitSpawnTolerance = TILE_SIZE * 0.5f;

    const glm::vec2 pipePos = pipe.GetPosition();
    const glm::vec2 pipeSize = pipe.GetSize();
    const float playerLeft = playerPos.x;
    const float playerRight = playerPos.x + playerSize.x;
    const float playerTop = playerPos.y;
    const float playerBottom = playerPos.y + playerSize.y;
    const float playerCenterX = playerPos.x + playerSize.x * 0.5f;
    const float playerCenterY = playerPos.y + playerSize.y * 0.5f;

    const bool centerInsidePipeX =
        playerCenterX >= pipePos.x &&
        playerCenterX <= pipePos.x + pipeSize.x;
    const bool centerInsidePipeY =
        playerCenterY >= pipePos.y &&
        playerCenterY <= pipePos.y + pipeSize.y;

    if (centerInsidePipeX && centerInsidePipeY) {
        return true;
    }

    const std::string opening = pipe.GetOpening();
    if (opening == "up") {
        return centerInsidePipeX &&
               std::abs(playerBottom - pipePos.y) <= kPipeExitSpawnTolerance;
    }
    if (opening == "down") {
        return centerInsidePipeX &&
               std::abs(playerTop - (pipePos.y + pipeSize.y)) <= kPipeExitSpawnTolerance;
    }
    if (opening == "left") {
        return centerInsidePipeY &&
               std::abs(playerRight - pipePos.x) <= kPipeExitSpawnTolerance;
    }
    if (opening == "right") {
        return centerInsidePipeY &&
               std::abs(playerLeft - (pipePos.x + pipeSize.x)) <= kPipeExitSpawnTolerance;
    }

    return false;
}

const char* StarBGMName(bool debugStar) {
    return debugStar ? kDebugStarmanBGMName : kStarmanBGMName;
}
}

void GameManager::ConfigurePlayer() {
    m_Player.SetControls(Player::DefaultControls());
    m_Player.SetDebugEnabled(m_DebugMode); // 與目前 Debug mode 同步
}

void GameManager::ResetPlayerForNewGame() {
    ConfigurePlayer();
    m_Player.SetVisualProfile(Player::VisualProfile::Mario);
    m_Player.ResetForNewGame();
}

void GameManager::ConfigurePlayerForCurrentSession() {
    ConfigurePlayer();
    const auto profile = m_Session.GetCurrentPlayerSlot() == PlayerSlot::Luigi
        ? Player::VisualProfile::Luigi
        : Player::VisualProfile::Mario;
    m_Player.SetVisualProfile(profile);
}

void GameManager::SetPlayerSpawnPosition(glm::vec2 position) {
    m_Player.SetSpawnPosition(position);
}

void GameManager::UpdateCameraForPlayer() {
    m_Camera.Update(m_Player.GetPosition().x, static_cast<float>(m_Level.levelWidth));
}

int GameManager::FindLevelIndexByName(const std::string& levelName) const {
    for (int i = 0; i < static_cast<int>(kLevelChain.size()); ++i) {
        if (kLevelChain[i].levelName == levelName) {
            return i;
        }
    }
    return -1;
}

std::string GameManager::WorldLabelForLevelName(const std::string& levelName) const {
    const int levelIndex = FindLevelIndexByName(levelName);
    return levelIndex >= 0 ? kLevelChain[levelIndex].worldLabel : levelName;
}

std::string GameManager::CurrentPlayerWorldLabel() const {
    const auto& progress = m_Session.CurrentPlayer();
    const std::string levelName = progress.levelName.empty()
        ? m_SelectedInitialLevelName
        : progress.levelName;
    return WorldLabelForLevelName(levelName);
}

void GameManager::SyncCurrentLevelFromSession() {
    const auto& progress = m_Session.CurrentPlayer();
    const int levelIndex = FindLevelIndexByName(progress.levelName);
    m_CurrentLevelIndex = levelIndex;
    m_SelectedWorldLabel = CurrentPlayerWorldLabel();
}

std::string GameManager::CurrentPlayerIconPath() const {
    return m_Session.GetCurrentPlayerSlot() == PlayerSlot::Luigi
        ? "player/LuigiSprites/native/small_luigi/frame_00_16x16.png"
        : "player/Mario/right/Walk1/Walk1.png";
}

void GameManager::RefreshTopScoreFromSession() {
    for (int i = 0; i < m_Session.GetPlayerCount(); ++i) {
        m_TopScore = std::max(m_TopScore, m_Session.GetPlayerProgress(i).score);
    }
}

// ─── Start ────────────────────────────────────────────────────────────────

void GameManager::Start() {
    ConfigurePlayer();
    EnterTitleScreen();
}

// ─── Update ───────────────────────────────────────────────────────────────

void GameManager::Update() {
    const float rawDt = Util::Time::GetDeltaTimeMs() / 1000.0f;
    const float dt = std::min(rawDt, MAX_FRAME_DT);

    if (rawDt > MAX_FRAME_DT) {
        LOG_WARN("Large frame dt detected: {}s, clamped to {}s", rawDt, dt);
    }

    if (HandleLevelCheatShortcut()) {
        return;
    }

    switch (m_FlowState) {
        case FlowState::Title:
            UpdateTitle(dt);
            break;
        case FlowState::LevelIntro:
            UpdateLevelIntro(dt);
            break;
        case FlowState::IntroCutscene:
            UpdateIntroCutscene(dt);
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

    // 標題畫面也可以按 ESC 進入暫停選單（可調音量 / 看按鍵說明 / 離開遊戲）。
    if (UpdatePauseMenu()) {
        return;
    }

    if (Util::Input::IsKeyDown(Util::Keycode::UP) ||
        Util::Input::IsKeyDown(Util::Keycode::W) ||
        Util::Input::IsKeyDown(Util::Keycode::DOWN) ||
        Util::Input::IsKeyDown(Util::Keycode::S)) {
        m_TitleSelectionIndex = 1 - m_TitleSelectionIndex;
        ClearOverlayObjects();
        BuildTitleOverlay();
    }

    if (Util::Input::IsKeyDown(Util::Keycode::RETURN) ||
        Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        m_SelectedPlayerCount = m_TitleSelectionIndex == 0 ? 1 : 2;
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

void GameManager::UpdateIntroCutscene(float dt) {
    if (m_Player.GetState() == Player::State::EnteringPipe) {
        if (m_Player.IsAnimationFinished()) {
            ChangeLevel(m_PendingLevel, m_PendingSpawn);
            m_FlowState = FlowState::Playing;
            return;
        }

        m_Player.Update(dt);
        UpdateCameraForPlayer();
        DrawScene(true);
        return;
    }

    if (m_Player.GetState() == Player::State::IntroAutoWalk) {
        m_Player.Update(dt);
        UpdateCameraForPlayer();
        CheckBlockCollision(m_Player);
        m_Player.ClampToCameraBounds(m_Camera.GetX());

        if (m_Player.IsIntroAutoWalkFinished()) {
            PipeBlock* pipe = FindIntroCutscenePipe();
            if (!pipe) {
                LOG_WARN("Intro auto-enter-pipe cutscene has no matching target pipe. Falling back to Playing.");
                m_FlowState = FlowState::Playing;
                DrawScene(true);
                return;
            }

            m_PendingLevel = pipe->GetDestinationLevel();
            m_PendingSpawn = pipe->GetDestinationSpawn();
            const float duration = std::max(
                kPipeTransitionDuration,
                m_Level.introCutscene ? m_Level.introCutscene->pipeEntryDuration : 1.0f);
            m_Player.StartPipeEntry(pipe->GetPosition(), pipe->GetSize(), pipe->GetOpening(), duration);
            m_Audio.PauseBGM();
            m_Audio.PlaySFX("pipe");
        }

        DrawScene(true);
        return;
    }

    LOG_WARN("Intro cutscene entered with unsupported player state. Falling back to Playing.");
    m_FlowState = FlowState::Playing;
    DrawScene(true);
}

void GameManager::UpdatePlaying(float dt) {
    // 暫停選單（ESC 開啟 / 操作）；回傳 true 表示本幀由暫停接管，不更新世界。
    if (UpdatePauseMenu()) {
        return;
    }

    if (m_Player.GetState() == Player::State::EnteringPipe && m_Player.IsAnimationFinished()) {
        ChangeLevel(m_PendingLevel, m_PendingSpawn);
        return;
    }

    // 鑽出水管動畫播完：恢復正常操作狀態。
    if (m_Player.GetState() == Player::State::ExitingPipe && m_Player.IsAnimationFinished()) {
        m_Player.FinishPipeExit();
    }

    // 1. 更新玩家邏輯
    m_Player.Update(dt);

    if (m_Player.ConsumeJumpEvent()) {
        const std::string jumpSfx = (m_Player.GetForm() == Player::Form::SMALL)
                                            ? "jump_small" : "jump_super";
        m_Audio.PlaySFX(jumpSfx);
    }

    bool debugStarEvent = false;
    if (m_Player.ConsumeStarStartedEvent(debugStarEvent)) {
        if (!m_LevelCleared) {
            m_Audio.PlayEventBGM(StarBGMName(debugStarEvent));
        }
        LOG_INFO("Star started (debug={}), playing '{}' BGM.",
                 debugStarEvent,
                 StarBGMName(debugStarEvent));
    }

    if (m_Player.ConsumeStarEndedEvent(debugStarEvent)) {
        if (!m_LevelCleared) {
            m_Audio.EndEventBGM(StarBGMName(debugStarEvent));
        }
        LOG_INFO("Star ended, restoring managed BGM.");
    }

    // ── 死亡偵測：玩家剛進入 Dying 狀態時，以最高優先序播放死亡 BGM ──
    const bool playerIsDyingNow = m_Player.IsDying();
    if (playerIsDyingNow && !m_PlayerWasDying) {
        m_DeathSequenceTimer = 0.0f;
        m_Audio.PlayEventBGM("death", 0); // 死亡 BGM 播一次
    } else if (playerIsDyingNow) {
        m_DeathSequenceTimer += dt;
    } else if (!m_WaitingForVoidDeath) {
        // 墜入虛空的停留計時由虛空判定區段自行維護，這裡不要歸零
        m_DeathSequenceTimer = 0.0f;
    }
    m_PlayerWasDying = playerIsDyingNow;

    // 變身動畫結束時套用待定的傷害無敵（縮小後才開始計時）
    if (!m_Player.IsTransforming() && m_PendingDamageInvincibility > 0.0f) {
        m_Player.StartDamageInvincibility(m_PendingDamageInvincibility);
        m_PendingDamageInvincibility = 0.0f;
        LOG_INFO("Post-transform damage invincibility started.");
    }

    if (m_Player.GetState() == Player::State::EnteringPipe ||
        m_Player.GetState() == Player::State::ExitingPipe) {
        UpdateCameraForPlayer();
        DrawScene(true);
        return;
    }

    if (m_Player.IsTransforming()) {
        UpdateCameraForPlayer();
        DrawScene(true);
        return;
    }

    // 如果玩家正在播放死亡動畫（含墜入虛空的停留），停止更新世界其他物件（凍結畫面）
    if (!m_Player.IsDying() && !m_WaitingForVoidDeath) {
        for (auto& enemy : m_Enemies) {
            // 翻轉死亡中的 Koopa 仍需要更新物理（飛出畫面）
            auto* koopa = dynamic_cast<Koopa*>(enemy.get());
            if (koopa && koopa->IsDying()) {
                koopa->Update(dt);
                continue;
            }
            // 食人花：每幀同步玩家位置與尺寸，供管口站立判定使用
            auto* piranha = dynamic_cast<PiranhaPlant*>(enemy.get());
            if (piranha) {
                piranha->SetPlayerBounds(m_Player.GetPosition(), m_Player.GetSize());
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
            const glm::vec2 playerPos = m_Player.GetPosition();
            const glm::vec2 playerSize = m_Player.GetSize();
            const bool movingLeft = m_Player.IsFacingLeft();
            glm::vec2 spawnPos = {
                movingLeft ? playerPos.x - 10.0f : playerPos.x + playerSize.x - 6.0f,
                playerPos.y + 12.0f
            };
            SpawnFireball(spawnPos, movingLeft);
        }

        UpdateCameraForPlayer();

        // 2.5 鏡頭觸發：檢查是否有新敵人進入鏡頭範圍
        CheckEnemySpawnQueue();

        // 3. 執行碰撞偵測 (包含方塊與敵人)
        if (m_Player.IsAlive()) {
            CheckBlockCollision(m_Player);
        }

        bool pipeTransitionStarted = false;
        if (m_Player.IsAlive() && CheckPipeTransition(m_Player)) {
            pipeTransitionStarted = true;
        }

        if (pipeTransitionStarted) {
            UpdateCameraForPlayer();
        } else {
            CheckEnemyBlockCollision();
            CheckEnemyEnemyCollision();
            CheckItemBlockCollision();
            CheckItemCollision();
            CheckStompCollision(m_Player, m_PendingDamageInvincibility);
            CheckShellEnemyCollision();
            CheckFireballCollision();
            CheckFlagCollision(m_Player);
        }

        // 4. 玩家左界夾制：確保玩家不會走到鏡頭左邊界之外（棘輪規則的配套）
        m_Player.ClampToCameraBounds(m_Camera.GetX());

        // 5. 中繼點偵測：更新玩家已達成的最後中繼點
        if (m_Player.IsAlive()) {
            UpdateCheckpoints(m_Player);
        }
    }

    // ─── 虛空掉落判定 (Kill Z) ───
    const float killZ = m_Level.levelHeight + 50.0f;

    // Time Up 等待死亡動畫時：玩家落出畫面後進 TIME UP overlay，不走一般生命扣除
    if (m_WaitingForTimeUpDeath) {
        if (m_Player.GetPosition().y > killZ && m_DeathSequenceTimer >= kDeathSequenceMinDuration) {
            m_WaitingForTimeUpDeath = false;
            EnterTimeUp();
            return;
        }
        DrawScene(true);
        return;
    }

    if (m_Player.IsAlive() && m_Player.GetPosition().y > killZ) {
        // (A) 受傷後墜落：死亡 BGM 已於 Dying 偵測時播放，沿用既有停留流程
        if (m_Player.IsDying()) {
            if (m_DeathSequenceTimer < kDeathSequenceMinDuration) {
                DrawScene(true);
                return;
            }
            m_Player.SetAlive(false);
            LOG_INFO("Player fell into the void after hit! Life lost. pos=({:.2f},{:.2f}) killZ={:.2f}",
                     m_Player.GetPosition().x, m_Player.GetPosition().y, killZ);
            HandleLifeLost();
            return;
        }

        // (B) 未受傷直接墜入虛空：補播死亡 BGM，並停留與受傷死亡相同的時間
        if (!m_WaitingForVoidDeath) {
            m_WaitingForVoidDeath = true;
            m_DeathSequenceTimer = 0.0f;
            m_Audio.PlayEventBGM("death", 0); // 死亡 BGM 播一次
            LOG_INFO("Player fell into the void! Starting death sequence. pos=({:.2f},{:.2f}) killZ={:.2f}",
                     m_Player.GetPosition().x, m_Player.GetPosition().y, killZ);
        }
        m_DeathSequenceTimer += dt;
        if (m_DeathSequenceTimer < kDeathSequenceMinDuration) {
            DrawScene(true);
            return;
        }
        m_WaitingForVoidDeath = false;
        m_Player.SetAlive(false);
        LOG_INFO("Void death sequence finished. Life lost.");
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
        if (!m_HurryUpTriggered && m_TimeRemaining > 0.0f && m_TimeRemaining < 100.0f) {
            m_HurryUpTriggered = true;
            m_Audio.SetHurryUp(true);
            LOG_INFO("Hurry-up BGM triggered. timeRemaining={}", m_TimeRemaining);
        }
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
    // GAME OVER BGM 播完後再回標題
    if (m_StateTimer >= kGameOverDuration) {
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
    constexpr int COUNTDOWN_TICK_DURATION_MS = 120;

    m_LevelClearTransitionTimer += dt;
    if (!m_LevelClearCastleFlagSpawned &&
        m_LevelClearTransitionTimer >= kLevelClearBgmDuration) {
        m_LevelClearCastleFlagSpawned = true;
        if (CurrentPlayerWorldLabel() != "1-3") {
            SpawnCastleFlag(m_LevelClearPlayer ? m_LevelClearPlayer->GetCastleDoorX() : m_Player.GetCastleDoorX(),
                            m_FlagBottomY,
                            m_LevelClearCastleFlagBaseTiles);
        }
    }

    // 注意：遊玩時 m_TimeRemaining 以 dt×2.5 扣減，結算開始時幾乎必帶小數。
    // 必須把殘餘小數（< 1 單位）視為 0，否則 actualDeduct = min(n, int(0.x)) = 0
    // 會永遠扣不完，卡死在結算狀態（1-1 過關後進不了 1-2 的原因）。
    if (m_TimeRemaining >= 1.0f) {
        m_CountdownAccum += dt * TIME_DRAIN_PER_SECOND;
        const int unitsToDeduct = static_cast<int>(m_CountdownAccum);
        if (unitsToDeduct > 0) {
            m_CountdownAccum -= static_cast<float>(unitsToDeduct);

            const int actualDeduct = std::min(unitsToDeduct, static_cast<int>(m_TimeRemaining));
            m_TimeRemaining -= static_cast<float>(actualDeduct);
            m_Session.AddScore(actualDeduct * static_cast<int>(SCORE_PER_UNIT));

            // countdown_tick.mp3 是 2 秒檔；結算只取短促起音，避免高頻 tick 佔滿 SFX channels。
            m_Audio.PlaySFX("countdown_tick", 0, COUNTDOWN_TICK_DURATION_MS);

            // 扣完整數單位後若只剩小數，直接歸零，下一幀進入停頓
            if (m_TimeRemaining < 1.0f) {
                m_TimeRemaining = 0.0f;
            }
        }
    } else {
        m_TimeRemaining = 0.0f;
        if (m_LevelClearTransitionTimer >= kLevelClearBgmDuration) {
            // 時間已歸零且過關 BGM 已播完，進入停頓狀態
            EnterLevelClearPause();
            return;
        }
    }

    // 城堡小旗在過關 BGM 播完後才從塔頂冒出
    if (m_LevelClearCastleFlagSpawned) {
        UpdateCastleFlagRaise(dt);
    }

    // 每幀更新畫面（含 HUD 時間與分數的即時動態顯示）
    DrawScene(true);
}

// 結算結束後停頓 1 秒，再進下一關
void GameManager::UpdateLevelClearPause(float dt) {
    m_StateTimer += dt;
    if (m_LevelClearCastleFlagSpawned) {
        UpdateCastleFlagRaise(dt);
    }
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
    m_CastleFlag.reset();
    // 效能優化：清空複用的暫存容器（不釋放已分配的 capacity）
    m_TmpBlocksToRemove.clear();
    m_TmpItemsToRemove.clear();
    m_TmpFireballsToRemove.clear();
    m_Background.reset();
    m_Renderer = Util::Renderer();
    ResetCombo();
}

void GameManager::ClearOverlayObjects() {
    for (auto& obj : m_OverlayObjects) {
        m_Renderer.RemoveChild(obj);
    }
    m_OverlayObjects.clear();
}

bool GameManager::HandleLevelCheatShortcut() {
    // Debug mode 關閉時，1~8 作弊功能鍵全部停用
    if (!m_DebugMode) {
        return false;
    }

    // 按鈕 8 作弊：所有玩家生命值 +3（不跳關、不重置場景，回傳 false 讓遊戲照常進行）
    if (Util::Input::IsKeyDown(Util::Keycode::NUM_8)) {
        for (int i = 0; i < m_Session.GetPlayerCount(); ++i) {
            auto& progress = m_Session.GetPlayerProgress(i);
            progress.lives += 3;
        }
        LOG_INFO("Cheat activated: all players lives +3 (players={})",
                 m_Session.GetPlayerCount());
        return false;
    }

    int targetIndex = -1;

    if (Util::Input::IsKeyDown(Util::Keycode::NUM_1)) {
        targetIndex = 0;
    } else if (Util::Input::IsKeyDown(Util::Keycode::NUM_2)) {
        targetIndex = 1;
    } else if (Util::Input::IsKeyDown(Util::Keycode::NUM_3)) {
        targetIndex = 2;
    }

    if (targetIndex < 0 || targetIndex >= static_cast<int>(kLevelChain.size())) {
        return false;
    }

    const auto& target = kLevelChain[targetIndex];

    LOG_INFO("Level cheat activated: key={} level='{}' world='{}'",
             targetIndex + 1,
             target.levelName,
             target.worldLabel);

    // 先把 4/5/6 這類 debug 形態切換寫回 session，避免 1/2/3 跳關後
    // EnterLevelIntro() 又用舊的 progress.form 覆蓋回原狀態。
    SavePlayerProgress();

    // 保留統計的跳關：模仿 AdvanceToNextLevel，但直接跳到 targetIndex，
    // 刻意不呼叫 StartNewGame()/ResetNewGame()，以免歸零金幣、分數與命數。
    m_CurrentLevelIndex = targetIndex;
    auto& progress = m_Session.CurrentPlayer();
    progress.levelName = kLevelChain[targetIndex].levelName;
    progress.currentLevel = std::string{}; // 退回鏈錨點
    progress.checkpoint = std::nullopt;
    // 保留 score / coins；但若已 game over（生命 <= 0），補回 3 條以便繼續遊玩
    if (progress.lives <= 0) {
        progress.lives = 3;
    }
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;
    EnterLevelIntro();
    return true;
}

void GameManager::StartNewGame() {
    ResetSceneObjects();
    m_Session.ResetNewGame(m_SelectedPlayerCount);
    ResetPlayerForNewGame();

    // 新遊戲：清空中繼點與中繼點重生覆蓋
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;

    // 從標題畫面開始的新遊戲一律從關卡鏈第一關開始。
    // Debug 跳關只影響當前 session，不能成為下一場新遊戲的起點。
    m_CurrentLevelIndex = 0;
    m_SelectedInitialLevelName = kLevelChain.front().levelName;

    // 套用關卡鏈的關卡名稱到目前關卡欄位
    m_SelectedWorldLabel = kLevelChain[m_CurrentLevelIndex].worldLabel;
    for (int i = 0; i < m_Session.GetPlayerCount(); ++i) {
        auto& progress = m_Session.GetPlayerProgress(i);
        progress.levelName = kLevelChain[m_CurrentLevelIndex].levelName;
        progress.currentLevel = std::string{}; // 退回鏈錨點
        progress.checkpoint = std::nullopt;
    }
    ConfigurePlayerForCurrentSession();

    EnterLevelIntro();
    LOG_INFO("New game started. players={} level index={} name='{}' world='{}'",
             m_Session.GetPlayerCount(),
             m_CurrentLevelIndex,
             kLevelChain[m_CurrentLevelIndex].levelName,
             m_SelectedWorldLabel);
}

void GameManager::EnterLevelIntro() {
    ResetSceneObjects();
    ConfigurePlayerForCurrentSession();
    SyncCurrentLevelFromSession();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 400.0f;
    m_LevelCleared = false;
    m_LevelClearTransitionTimer = 0.0f;
    m_LevelClearCastleFlagSpawned = false;
    m_LevelClearPlayer = &m_Player;
    m_WaitingForTimeUpDeath = false;
    m_WaitingForVoidDeath = false;
    m_PlayerWasDying = false;
    m_DeathSequenceTimer = 0.0f;
    m_HurryUpTriggered = false;
    m_Paused = false;          // 換關時清空暫停
    m_PauseSelectionIndex = 0; // 暫停選單游標歸位（overlay 由 ClearOverlayObjects 清除）
    m_PendingDamageInvincibility = 0.0f; // 清空待定無敵
    m_Audio.ResetBGMState();

    const auto& currentProgress = m_Session.CurrentPlayer();
    // 重生時優先載入玩家實際所在的子關卡（含水管進入的子關卡）；
    // 為空時退回關卡鏈錨點 levelName，再退回標題選關名稱。
    const std::string anchorLevel = currentProgress.levelName.empty()
        ? m_SelectedInitialLevelName
        : currentProgress.levelName;
    const std::string levelToLoad = currentProgress.currentLevel.empty()
        ? anchorLevel
        : currentProgress.currentLevel;
    m_LastCheckpoint = currentProgress.checkpoint;
    m_CheckpointRespawnOverride = currentProgress.checkpoint;

    LoadLevel(MakeLevelPath(levelToLoad));
    ApplyPlayerProgress();

    // 若有中繼點重生覆蓋，套用後清除（只用一次）
    const bool isCheckpointRespawn = m_CheckpointRespawnOverride.has_value();
    if (isCheckpointRespawn) {
        SetPlayerSpawnPosition(*m_CheckpointRespawnOverride);
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

    // 中繼點重生：仿原版 NES——只修剪「鏡頭左緣以左（已被越過、看不到）」的敵人，
    // 畫面內可見的敵人保留下來（避免重生點周圍的敵人整批消失）；
    // 另外硬性保留中繼點水平 3 格安全區，避免玩家重生即被緊鄰的敵人撞死。
    if (isCheckpointRespawn) {
        const float viewLeft = m_Camera.GetX();
        const float checkpointX = m_Player.GetPosition().x;
        const auto removeBegin = std::remove_if(
            m_EnemySpawnQueue.begin(), m_EnemySpawnQueue.end(),
            [&](const ObjectData& d) {
                const bool alreadyPassed = d.x < viewLeft;
                const bool insideCheckpointSafeZone =
                    std::abs(d.x - checkpointX) <= kCheckpointEnemySafeRadius;
                return alreadyPassed || insideCheckpointSafeZone;
            });
        const auto removedCount = std::distance(removeBegin, m_EnemySpawnQueue.end());
        m_EnemySpawnQueue.erase(removeBegin, m_EnemySpawnQueue.end());
        LOG_INFO("Checkpoint respawn: pruned {} enemy spawns left of viewLeft={} or within {} px of checkpointX={}",
                 removedCount,
                 viewLeft,
                 kCheckpointEnemySafeRadius,
                 checkpointX);
    }
    BuildScene();
    const std::string currentWorldLabel = CurrentPlayerWorldLabel();
    m_SelectedWorldLabel = currentWorldLabel;
    m_Hud.Init(m_Renderer, currentWorldLabel, m_Session.GetCurrentPlayerName());
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
    m_Hud.Init(m_Renderer, CurrentPlayerWorldLabel(), m_Session.GetCurrentPlayerName());

    m_LevelCleared = false;
    m_LevelClearPlayer = &m_Player;
    m_WaitingForTimeUpDeath = false;
    m_WaitingForVoidDeath = false;
    m_Paused = false;          // 確保進入遊玩狀態時不殘留暫停
    m_PauseSelectionIndex = 0; // 暫停選單游標歸位（overlay 由 ClearOverlayObjects 清除）
    m_PendingDamageInvincibility = 0.0f; // 清空待定無敵

    m_FlowState = FlowState::Playing;
    m_PlayerWasDying = false; // 重置死亡偵測狀態
    m_DeathSequenceTimer = 0.0f;
    // 開始播放關卡 BGM（依 LoadLevel 時決定的主題）
    m_Audio.SetAreaBGM(m_LevelBGMName);
    if (TryEnterIntroCutscene()) {
        return;
    }
    LOG_INFO("Entered playing state.");
}

bool GameManager::TryEnterIntroCutscene() {
    if (!m_Level.introCutscene.has_value()) {
        return false;
    }

    const IntroCutsceneData& intro = *m_Level.introCutscene;
    if (intro.type != "autoEnterPipe") {
        LOG_WARN("Unsupported introCutscene type='{}'. Falling back to normal play.", intro.type);
        return false;
    }

    PipeBlock* pipe = FindIntroCutscenePipe();
    if (!pipe) {
        LOG_WARN("autoEnterPipe introCutscene targetLevel='{}' not found. Falling back to normal play.",
                 intro.pipeTargetLevel);
        return false;
    }

    const float targetX = pipe->GetPosition().x - m_Player.GetSize().x;
    const float walkSpeed = intro.walkSpeed > 0.0f ? intro.walkSpeed : PLAYER_MAX_WALK_SPEED;
    m_Player.StartIntroAutoWalk(targetX, walkSpeed);
    m_FlowState = FlowState::IntroCutscene;
    LOG_INFO("Entered intro cutscene: type='{}' targetLevel='{}'",
             intro.type,
             intro.pipeTargetLevel);
    return true;
}

PipeBlock* GameManager::FindIntroCutscenePipe() const {
    if (!m_Level.introCutscene.has_value()) {
        return nullptr;
    }

    const std::string& targetLevel = m_Level.introCutscene->pipeTargetLevel;
    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;

        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe || !pipe->IsEnterable()) continue;
        if (pipe->GetDestinationLevel() == targetLevel) {
            return pipe;
        }
    }

    return nullptr;
}

void GameManager::EnterTitleScreen() {
    ResetSceneObjects();
    RefreshTopScoreFromSession();
    m_LevelCleared = false;
    m_LevelClearPlayer = &m_Player;
    m_WaitingForTimeUpDeath = false;
    m_WaitingForVoidDeath = false;
    m_PlayerWasDying = false;
    m_DeathSequenceTimer = 0.0f;
    m_StateTimer = 0.0f;
    m_LevelClearTransitionTimer = 0.0f;
    m_LevelClearCastleFlagSpawned = false;
    m_TitleSelectionIndex = 0;
    m_SelectedPlayerCount = 1;
    m_CurrentLevelIndex = -1;
    m_SelectedInitialLevelName = kLevelChain.front().levelName;
    m_SelectedWorldLabel = kLevelChain.front().worldLabel;
    ResetPlayerForNewGame();
    // 回標題畫面時停止所有音樂並清空 BGM 狀態
    m_Audio.ResetBGMState();

    // 仿原版 NES：標題畫面以 1-1 開頭實景當背景（藍天、地面、馬力歐站在出生點），
    // logo 與選單疊在場景上。標題狀態不更新世界邏輯（UpdateTitle 只 DrawScene），
    // 敵人生成佇列只在 Playing 的 CheckEnemySpawnQueue 觸發，不會跑出怪。
    m_Camera.Reset();
    LoadLevel(MakeLevelPath("1-1_ground_1"));
    m_Player.SetOnGround(true);
    BuildScene();
    m_Hud.Init(m_Renderer, m_SelectedWorldLabel, "MARIO");
    const auto& marioProgress = m_Session.GetPlayerProgress(0);
    m_Hud.Update(marioProgress.score, 0, -1);
    BuildTitleOverlay();

    m_FlowState = FlowState::Title;
    LOG_INFO("Entered title screen.");
}

void GameManager::EnterTimeUp() {
    ResetSceneObjects();
    m_StateTimer = 0.0f;
    m_TimeRemaining = 0.0f;
    m_LevelCleared = false;
    m_LevelClearTransitionTimer = 0.0f;
    m_LevelClearCastleFlagSpawned = false;
    m_PlayerWasDying = false;
    m_DeathSequenceTimer = 0.0f;

    m_Hud.Init(m_Renderer, CurrentPlayerWorldLabel(), m_Session.GetCurrentPlayerName());
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
    m_LevelClearTransitionTimer = 0.0f;
    m_LevelClearCastleFlagSpawned = false;
    m_WaitingForTimeUpDeath = false;
    m_WaitingForVoidDeath = false;
    m_PlayerWasDying = false;
    m_DeathSequenceTimer = 0.0f;
    m_HurryUpTriggered = false;

    // 建立黑底 + GAME OVER 文字 overlay
    BuildGameOverOverlay();

    m_FlowState = FlowState::GameOver;
    // 播放 Game Over 音效 BGM（播一次，不循環）
    m_Audio.PlayEventBGM("game_over", 0);
    LOG_INFO("Entered GAME OVER state.");
}

void GameManager::EnterLevelClearTransition() {
    SavePlayerProgress();
    // 不在此清空場景——保留畫面背景，等時間結算結束後再切關
    // （ResetSceneObjects 會在進下一關 EnterLevelIntro 時呼叫）
    m_LevelClearTransitionTimer = 0.0f;
    m_CountdownAccum = 0.0f;
    m_LevelClearCastleFlagSpawned = false;
    // 原版 NES 的時間結算沒有任何提示文字，直接在遊戲畫面上
    // TIME 遞減、SCORE 遞增（見 UpdateLevelClearTransition）
    m_FlowState = FlowState::LevelClearTransition;
    // 播放過關 BGM（播一次，不循環）
    m_Audio.PlayEventBGM("level_clear", 0);
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
    SavePlayerProgress();

    auto& progress = m_Session.CurrentPlayer();
    const int currentIndex = FindLevelIndexByName(progress.levelName);
    const int nextIndex = (currentIndex >= 0 ? currentIndex : m_CurrentLevelIndex) + 1;

    if (nextIndex >= static_cast<int>(kLevelChain.size())) {
        // 已是最後一關，回標題
        LOG_INFO("All levels cleared! Returning to title.");
        // 重置進度 index，讓下次開始新遊戲從第一關開始
        m_CurrentLevelIndex = -1;
        EnterTitleScreen();
        DrawScene(false);
        return;
    }

    // 推進到下一關
    progress.levelName = kLevelChain[nextIndex].levelName;
    progress.currentLevel = std::string{}; // 退回鏈錨點，新關卡從頭開始
    progress.checkpoint = std::nullopt;

    // 換關時清空中繼點（新關卡從頭開始）
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;

    LOG_INFO("Advanced player='{}' to level index={} name='{}' world='{}'",
             m_Session.GetCurrentPlayerName(),
             nextIndex,
             kLevelChain[nextIndex].levelName,
             kLevelChain[nextIndex].worldLabel);

    EnterLevelIntro();
    DrawScene(false);
}

// 把 NES 像素座標換算成螢幕座標（視窗 960×720、GAME_SCALE=3）：
// 可視世界寬 320px 比 NES 的 256px 多 64px，版面水平置中（左右各 32 世界px 邊距）；
// 垂直方向 240px 剛好填滿，無邊距。傳入元素「中心」的 NES 座標。
static glm::vec2 NesToScreen(float nesX, float nesY) {
    const auto context = Core::Context::GetInstance();
    const float halfW = static_cast<float>(context->GetWindowWidth()) * 0.5f;
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;
    const float marginX = (static_cast<float>(context->GetWindowWidth()) / GAME_SCALE - 256.0f) * 0.5f;
    return {(nesX + marginX) * GAME_SCALE - halfW, halfH - nesY * GAME_SCALE};
}

void GameManager::BuildTitleOverlay() {
    // 仿原版 NES 標題版面（NES 座標）：
    //   logo        x 40..216, y 24..112 → 中心 (128, 68)
    //   (C)1985     右緣對齊 logo 右緣 216，y 116..124
    //   選單        文字左緣 x=88，y=144（1P）/ y=160（2P），蘑菇游標在 x≈76
    //   TOP- 000000 左緣 x=88，y=184
    constexpr int kTitleFontSize = 24; // NES 8px 字 × GAME_SCALE(3)

    AddOverlayImage("ui/title/logo.png", NesToScreen(128.0f, 80.0f), {GAME_SCALE, GAME_SCALE}, kTitleTextZ);
    AddOverlayText("(C)1985 NINTENDO", kTitleFontSize, NesToScreen(152.0f, 132.0f) + glm::vec2(0.0f, 4.0f), kTitleTextZ);

    const float cursorY = m_TitleSelectionIndex == 0 ? 148.0f : 164.0f;
    AddOverlayImage("ui/title/cursor.png",
                    NesToScreen(76.0f, cursorY),
                    {GAME_SCALE, GAME_SCALE},
                    kTitleTextZ);
    AddOverlayText("1 PLAYER GAME", kTitleFontSize, NesToScreen(140.0f, 148.0f), kTitleTextZ);
    AddOverlayText("2 PLAYER GAME", kTitleFontSize, NesToScreen(140.0f, 164.0f), kTitleTextZ);

    AddOverlayText("TOP- " + FormatScore6(m_TopScore), kTitleFontSize, NesToScreen(132.0f, 188.0f), kTitleTextZ);
}

std::vector<GameManager::PauseAction> GameManager::PauseMenuActions() const {
    std::vector<PauseAction> actions = {
        PauseAction::Resume, PauseAction::Volume, PauseAction::Controls, PauseAction::DebugMode};
    if (!m_PausedFromTitle) {
        actions.push_back(PauseAction::BackToTitle); // 標題畫面無需「回到主選單」
    }
    actions.push_back(PauseAction::Quit);
    return actions;
}

std::string GameManager::PauseActionLabel(PauseAction action) const {
    switch (action) {
        case PauseAction::Resume:      return "CONTINUE";
        case PauseAction::Volume:      return "VOLUME ( " + std::to_string(m_Audio.GetVolumePercent() / 10) + " )";
        case PauseAction::Controls:    return "CONTROLS";
        case PauseAction::DebugMode:   return std::string("DEBUG MODE  ") + (m_DebugMode ? "ON" : "OFF");
        case PauseAction::BackToTitle: return "BACK TO MENU";
        case PauseAction::Quit:        return "QUIT GAME";
    }
    return "";
}

bool GameManager::UpdatePauseMenu() {
    if (m_Paused) {
        // ── 二級「按鍵說明」畫面：ESC 返回主選單，其餘只渲染 ──
        if (m_PauseScreen == PauseScreen::Controls) {
            if (Util::Input::IsKeyDown(Util::Keycode::ESCAPE)) {
                m_PauseScreen = PauseScreen::Main;
                RebuildPauseOverlay();
            }
            m_Renderer.Update();
            return true;
        }

        // ── 主選單 ──
        // ESC：直接恢復遊戲
        if (Util::Input::IsKeyDown(Util::Keycode::ESCAPE)) {
            ResumePause();
            return true;
        }

        const auto actions = PauseMenuActions();
        const int count = static_cast<int>(actions.size());

        // 上/下方向鍵（含 W/S）移動游標，環繞
        const bool moveUp = Util::Input::IsKeyDown(Util::Keycode::UP) ||
                            Util::Input::IsKeyDown(Util::Keycode::W);
        const bool moveDown = Util::Input::IsKeyDown(Util::Keycode::DOWN) ||
                              Util::Input::IsKeyDown(Util::Keycode::S);
        if (moveUp || moveDown) {
            const int delta = moveDown ? 1 : -1;
            m_PauseSelectionIndex = (m_PauseSelectionIndex + delta + count) % count;
            RebuildPauseOverlay();
        }

        const PauseAction current = actions[m_PauseSelectionIndex];

        // 左/右方向鍵（含 A/D）：在「音量」列調整音量，或在「DEBUG MODE」列切換開關
        const bool leftPressed = Util::Input::IsKeyDown(Util::Keycode::LEFT) ||
                                 Util::Input::IsKeyDown(Util::Keycode::A);
        const bool rightPressed = Util::Input::IsKeyDown(Util::Keycode::RIGHT) ||
                                  Util::Input::IsKeyDown(Util::Keycode::D);
        if (current == PauseAction::Volume && (leftPressed || rightPressed)) {
            m_Audio.SetVolumePercent(m_Audio.GetVolumePercent() + (rightPressed ? 10 : -10));
            RebuildPauseOverlay();
        } else if (current == PauseAction::DebugMode && (leftPressed || rightPressed)) {
            SetDebugMode(!m_DebugMode);
            RebuildPauseOverlay();
        }

        // Enter / Space：確認選中項
        if (Util::Input::IsKeyDown(Util::Keycode::RETURN) ||
            Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
            switch (current) {
                case PauseAction::Resume:
                    ResumePause();
                    break;
                case PauseAction::Volume:
                    break; // 用左右鍵調整
                case PauseAction::Controls:
                    m_PauseScreen = PauseScreen::Controls;
                    RebuildPauseOverlay();
                    break;
                case PauseAction::DebugMode:
                    SetDebugMode(!m_DebugMode); // Enter 也可切換
                    RebuildPauseOverlay();
                    break;
                case PauseAction::BackToTitle:
                    ClearOverlayObjects();
                    m_Paused = false;
                    EnterTitleScreen(); // 內含 ResetBGMState 與完整重置
                    break;
                case PauseAction::Quit:
                    m_QuitRequested = true;
                    LOG_INFO("Quit requested from pause menu.");
                    break;
            }
            return true;
        }

        m_Renderer.Update();
        return true;
    }

    // 未暫停時按 ESC：開啟暫停選單（本幀直接渲染後返回，不更新世界）
    if (Util::Input::IsKeyDown(Util::Keycode::ESCAPE)) {
        EnterPause();
        m_Renderer.Update();
        return true;
    }
    return false;
}

void GameManager::SetDebugMode(bool enabled) {
    m_DebugMode = enabled;
    m_Player.SetDebugEnabled(enabled); // 同步 Player 端的 4~7 變身作弊鍵
    LOG_INFO("Debug mode {}.", enabled ? "enabled" : "disabled");
}

void GameManager::EnterPause() {
    m_Paused = true;
    m_PausedFromTitle = (m_FlowState == FlowState::Title);
    m_PauseScreen = PauseScreen::Main;
    m_PauseSelectionIndex = 0;
    m_Audio.PauseBGM();
    m_Audio.PlaySFX("pause");
    // 暫停時 UpdateScorePopups 不會執行，浮動得分（z=25，在黑底 z=15 之上）會凍結
    // 並穿透暫停選單，故先隱藏；ResumePause 再還原仍存活的彈出。
    for (auto& p : m_ScorePopups) {
        if (p.obj) p.obj->SetVisible(false);
    }
    // RebuildPauseOverlay 會先 ClearOverlayObjects（清掉標題畫面既有的 logo / 選單），
    // 再建立暫停選單，避免兩層 overlay 疊在一起。
    RebuildPauseOverlay();
    LOG_INFO("Game paused. fromTitle={}", m_PausedFromTitle);
}

void GameManager::ResumePause() {
    ClearOverlayObjects();
    m_Paused = false;
    // 還原暫停時隱藏的浮動得分（計時已凍結，皆仍存活）
    for (auto& p : m_ScorePopups) {
        if (p.obj) p.obj->SetVisible(true);
    }
    m_Audio.ResumeBGM();
    // 若是在標題畫面暫停，恢復時要把標題的 logo 與選單重建回來
    if (m_PausedFromTitle) {
        BuildTitleOverlay();
    }
    LOG_INFO("Game resumed.");
}

void GameManager::BuildPauseOverlay() {
    constexpr int kPauseFontSize = 24;
    constexpr float kPauseBackdropZ = 15.0f;
    const std::string fontPath = MakeAssetPath("font/Super Mario Bros. NES.ttf");

    // 半透明黑底（沿用串場黑圖），讓選單在畫面上清楚可讀
    const auto context = Core::Context::GetInstance();
    const float windowW = static_cast<float>(context->GetWindowWidth());
    const float windowH = static_cast<float>(context->GetWindowHeight());
    AddOverlayImage("ui/title/black.png", {0.0f, 0.0f}, {windowW, windowH}, kPauseBackdropZ);

    // 標題 PAUSED（水平置中）
    AddOverlayText("PAUSED", fontPath, kPauseFontSize, NesToScreen(128.0f, 80.0f), kTitleTextZ);

    // 選項：以「動作清單」決定數量與文字（標題畫面少一項 BACK TO MENU）。
    // 文字物件是中心錨點，每行各自水平置中；游標跟著選中行左緣外側走。
    const auto actions = PauseMenuActions();
    const std::size_t n = actions.size();
    constexpr float kRowStep = 18.0f; // NES px
    const float firstY = 140.0f - kRowStep * static_cast<float>(n - 1) * 0.5f; // 以 y=140 為中心垂直置中

    std::vector<std::shared_ptr<Util::Text>> texts(n);
    for (std::size_t i = 0; i < n; ++i) {
        texts[i] = std::make_shared<Util::Text>(
            fontPath, kPauseFontSize, PauseActionLabel(actions[i]), Util::Color(255, 255, 255));
        const float centerY = NesToScreen(0.0f, firstY + kRowStep * static_cast<float>(i)).y;
        auto obj = std::make_shared<Util::GameObject>(texts[i], kTitleTextZ);
        obj->m_Transform.translation = {0.0f, centerY};
        obj->SetVisible(true);
        m_OverlayObjects.push_back(obj);
        m_Renderer.AddChild(obj);
    }

    // 游標：跟著「選中行」的左緣外側走，垂直對齊該行
    constexpr float kCursorGap = 14.0f;
    const float cursorHalfW = 8.0f * GAME_SCALE * 0.5f;
    const int sel = std::clamp(m_PauseSelectionIndex, 0, static_cast<int>(n) - 1);
    const float selectedHalfW = texts[sel]->GetSize().x * 0.5f;
    const float cursorX = -selectedHalfW - kCursorGap - cursorHalfW;
    const float cursorY = NesToScreen(0.0f, firstY + kRowStep * static_cast<float>(sel)).y;
    AddOverlayImage("ui/title/cursor.png", {cursorX, cursorY}, {GAME_SCALE, GAME_SCALE}, kTitleTextZ);
}

void GameManager::BuildControlsOverlay() {
    constexpr int kHeaderFontSize = 24;
    constexpr int kLineFontSize = 18;
    constexpr float kPauseBackdropZ = 15.0f;

    const auto context = Core::Context::GetInstance();
    const float windowW = static_cast<float>(context->GetWindowWidth());
    const float windowH = static_cast<float>(context->GetWindowHeight());
    AddOverlayImage("ui/title/black.png", {0.0f, 0.0f}, {windowW, windowH}, kPauseBackdropZ);

    // 標題
    AddOverlayText("CONTROLS", kHeaderFontSize, NesToScreen(128.0f, 40.0f), kTitleTextZ);

    // 每行「KEY - ACTION」皆水平置中。僅使用字型確定支援的字元（A-Z 0-9 空白 dash 括號）。
    // CHEATS 標題：Debug mode 關閉時加上「( DEBUG OFF )」提示。
    const std::string cheatsHeader = m_DebugMode ? "CHEATS" : "CHEATS ( DEBUG OFF )";
    struct Line { std::string text; float y; };
    const std::array<Line, 9> lines = {{
        {"BASIC",                           64.0f},
        {"MOVE - ARROWS OR A D",            82.0f},
        {"JUMP - SPACE UP OR W",            98.0f},
        {"RUN AND FIRE - Z",               114.0f},
        {"CROUCH - DOWN OR S",             130.0f},
        {cheatsHeader,                      156.0f},
        {"1 2 3 - WARP TO 1-1 1-2 1-3",    174.0f},
        {"4 5 6 7 - SMALL SUPER FIRE STAR", 190.0f},
        {"8 - ADD 3 LIVES",                206.0f},
    }};
    for (const auto& line : lines) {
        AddOverlayText(line.text, kLineFontSize, NesToScreen(128.0f, line.y), kTitleTextZ);
    }

    // 返回提示
    AddOverlayText("ESC - BACK", kLineFontSize, NesToScreen(128.0f, 228.0f), kTitleTextZ);
}

void GameManager::RebuildPauseOverlay() {
    ClearOverlayObjects();
    if (m_PauseScreen == PauseScreen::Controls) {
        BuildControlsOverlay();
    } else {
        BuildPauseOverlay();
    }
}

void GameManager::BuildLevelIntroOverlay() {
    // 仿原版 NES 串場畫面：純黑底 + 頂部 HUD + 置中 WORLD x-x + 馬力歐小圖示 × 命數。
    // 黑底 zIndex=15：蓋住已載入的場景（場景物件 z ≦ 10），
    // 但讓 HUD（z=20）與本 overlay 的文字／圖示（z=30）透出。
    constexpr float kIntroBackdropZ = 15.0f;
    constexpr int kIntroFontSize = 24; // NES 8px 字 × GAME_SCALE(3)

    const auto context = Core::Context::GetInstance();
    const float windowW = static_cast<float>(context->GetWindowWidth());
    const float windowH = static_cast<float>(context->GetWindowHeight());
    AddOverlayImage("ui/title/black.png", {0.0f, 0.0f}, {windowW, windowH}, kIntroBackdropZ);

    // 原版版面（NES 座標）：WORLD x-x 在 y≈84 列；玩家圖示 + 命數在 y≈112 列
    AddOverlayText("WORLD " + CurrentPlayerWorldLabel(), kIntroFontSize, NesToScreen(128.0f, 84.0f));

    AddOverlayImage(CurrentPlayerIconPath(),
                    NesToScreen(104.0f, 112.0f),
                    {GAME_SCALE, GAME_SCALE});

    std::ostringstream livesText;
    livesText << "x  " << m_Session.CurrentPlayer().lives;
    AddOverlayText(livesText.str(), kIntroFontSize, NesToScreen(138.0f, 112.0f));
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

void GameManager::AddOverlayText(const std::string& text, int fontSize, glm::vec2 position, float zIndex) {
    AddOverlayText(text, MakeAssetPath("font/Super Mario Bros. NES.ttf"), fontSize, position, zIndex);
}

void GameManager::AddOverlayText(const std::string& text, const std::string& fontPath,
                                 int fontSize, glm::vec2 position, float zIndex) {
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
    progress.checkpoint = m_LastCheckpoint;
}

void GameManager::ApplyPlayerProgress() {
    m_Player.SetForm(m_Session.CurrentPlayer().form);
}

void GameManager::HandleLifeLost() {
    SavePlayerProgress();
    m_Session.LoseLife();
    m_Session.CurrentPlayer().form = Player::Form::SMALL;

    if (m_Session.IsGameOver()) {
        LOG_INFO("Game over. Showing GAME OVER screen.");
        // 清空中繼點（Game Over 後回到標題，不保留進度）
        m_LastCheckpoint = std::nullopt;
        EnterGameOver();
        return;
    }

    if (m_Session.GetPlayerCount() > 1) {
        m_Session.SwitchToNextAlivePlayer();
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
    m_LevelBGMName = (m_Level.theme == Theme::Underground) ? "underground" : "overworld";

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
                obj.startDirection,
                static_cast<float>(std::max(0, obj.startOffsetTiles)) * TILE_SIZE));
        } else if (obj.type == "TreePlatform") {
            m_Blocks.push_back(std::make_shared<TreePlatformBlock>(pos, obj.segments));
        } else if (obj.type == "Wall") {
            m_Blocks.push_back(std::make_shared<WallBlock>(pos, m_ThemeAssets));
        } else if (obj.type == "Flag") {
            m_Blocks.push_back(std::make_shared<FlagBlock>(
                pos,
                obj.clearWalkTiles,
                obj.castleFlagBaseTiles));
        } else if (obj.type == "Coin" || obj.type == "CollectibleCoin") {
            m_Items.push_back(std::make_shared<LevelCoinItem>(pos, m_ThemeAssets));
        } else if (obj.type == "EnemySpawn") {
            // 不直接建立物件，改存進 queue，等鏡頭到達再生成
            m_EnemySpawnQueue.push_back(obj);
            LOG_INFO("EnemySpawn queued: type='{}' at ({}, {})", obj.enemyType, obj.x, obj.y);
        }
    }

    SetPlayerSpawnPosition(m_Level.playerSpawn);
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
    m_LevelClearTransitionTimer = 0.0f;
    m_LevelClearCastleFlagSpawned = false;

    // 換關（水管切換子關卡）：清空中繼點，並記住玩家現在實際所在的子關卡，
    // 讓死亡重生回到此子關卡（progress.levelName 維持為鏈錨點，供過關推進使用）。
    m_LastCheckpoint = std::nullopt;
    m_CheckpointRespawnOverride = std::nullopt;
    {
        auto& progress = m_Session.CurrentPlayer();
        progress.currentLevel = levelName;   // 死亡重生用：實際所在子關卡
        progress.checkpoint = std::nullopt;  // 進新子關卡，清空 checkpoint（之後由 UpdateCheckpoints 重新記錄）
    }

    m_Audio.ResetBGMState();
    LoadLevel(levelPath);
    m_Audio.SetAreaBGM(m_LevelBGMName);
    // 場景切換（水管進入子關卡）後若瑪利歐仍在星星無敵狀態，
    // 恢復無敵 BGM —— 無論身處哪個場景，無敵期間都要播無敵音樂。
    if (m_Player.IsStarInvincible()) {
        m_Audio.PlayEventBGM(StarBGMName(m_Player.IsDebugStarInvincible()));
    }

    if (spawnOverride.has_value()) {
        SetPlayerSpawnPosition(*spawnOverride);
        LOG_INFO("Player spawn override applied: {}", m_Player.GetPosition());
    }

    {
        const float lvW = static_cast<float>(m_Level.levelWidth);
        m_Camera.SetX(std::clamp(
            m_Player.GetPosition().x - m_Camera.GetViewWorldWidth() * 0.5f,
            0.0f, std::max(0.0f, lvW - m_Camera.GetViewWorldWidth())));
    }
    BuildScene();
    m_Hud.Init(m_Renderer, CurrentPlayerWorldLabel(), m_Session.GetCurrentPlayerName());

    // ─── 檢查是否起點對應水管，如果是則觸發鑽出動畫 ───
    const glm::vec2 pPos = m_Player.GetPosition();
    const glm::vec2 pSize = m_Player.GetSize();
    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;
        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe) continue;

        if (ShouldStartPipeExitForSpawn(*pipe, pPos, pSize)) {
            m_Player.StartPipeExit(pipe->GetPosition(), pipe->GetSize(), pipe->GetOpening(), kPipeTransitionDuration);
            break;
        }
    }
}

bool GameManager::CheckPipeTransition(Player& player) {
    const bool pressingDown = player.IsPressingDown();
    const bool pressingUp = player.IsPressingUp();
    const bool pressingLeft = player.IsPressingLeft();
    const bool pressingRight = player.IsPressingRight();

    for (const auto& block : m_Blocks) {
        if (block->GetType() != Block::Type::Pipe) continue;

        auto* pipe = dynamic_cast<PipeBlock*>(block.get());
        if (!pipe) continue;
        if (!pipe->CanEnter(player, pressingUp, pressingDown, pressingLeft, pressingRight)) continue;

        const std::string targetLevel = pipe->GetDestinationLevel();
        if (targetLevel.empty()) {
            LOG_WARN("Enterable pipe at {} has no target level.", pipe->GetPosition());
            return false;
        }

        m_PendingLevel = targetLevel;
        m_PendingSpawn = pipe->GetDestinationSpawn();
        player.StartPipeEntry(pipe->GetPosition(), pipe->GetSize(), pipe->GetOpening(), kPipeTransitionDuration);
        m_Audio.PauseBGM();
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
void GameManager::CheckStompCollision(Player& player, float& pendingDamageInvincibility) {
    if (!player.IsAlive()) return;

    const glm::vec2 pPos  = player.GetPosition();
    const glm::vec2 pSize = player.GetSize();
    const glm::vec2 pPrev = player.GetPreviousPosition();
    const glm::vec2 pVel  = player.GetVelocity();

    // 玩家落地時重置踩踏連殺 combo
    if (player.IsOnGround()) {
        ResetCombo();
    }

    for (auto& enemy : m_Enemies) {
        if (!enemy->CanCollide()) continue;

        const glm::vec2 ePos  = enemy->GetPosition();
        const glm::vec2 eSize = enemy->GetSize();

        // 矩形重疊判斷（AABB）
        if (!CollisionUtils::CheckAABB(pPos, pSize, ePos, eSize)) continue;

        if (player.IsStarInvincible()) {
            // 星星無敵：觸發翻轉死亡（使用 combo 計分）
            const bool flipLeft = (enemy->GetPosition().x + enemy->GetSize().x * 0.5f) >=
                                  (player.GetPosition().x + player.GetSize().x * 0.5f);
            if (auto* koopa = dynamic_cast<Koopa*>(enemy.get())) {
                koopa->Die(flipLeft);
            } else if (auto* goomba = dynamic_cast<Goomba*>(enemy.get())) {
                goomba->Die(flipLeft);
            } else {
                enemy->SetAlive(false);
                enemy->SetVisible(false);
            }
            {
                const int pts = NextComboScore();
                if (pts < 0) {
                    m_Session.AddLife();
                    SpawnScorePopup(-1, ePos);
                    m_Audio.PlaySFX("1up"); // 連殺 1UP 音效
                } else {
                    m_Session.AddScore(pts);
                    SpawnScorePopup(pts, ePos);
                }
                m_Audio.PlaySFX("kick"); // 星星無敵擊殺敵人音效
            }
            LOG_INFO("Star invincibility defeated enemy.");
            continue;
        }

        // ── 判斷是否為壓扁中的 Goomba（不具傷害性）──
        auto* goomba = dynamic_cast<Goomba*>(enemy.get());
        if (goomba && goomba->IsSquashed()) continue;

        // ── 判斷是否為靜止的龜殼 ──
        auto* koopa = dynamic_cast<Koopa*>(enemy.get());
        const bool isStationaryShell = koopa && koopa->IsStationaryShell();
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
                const float playerCenterX = pPos.x + pSize.x * 0.5f;
                const float shellCenterX = ePos.x + eSize.x * 0.5f;
                const bool kickLeft = Koopa::KickLeftFromContact(
                    playerCenterX, shellCenterX, player.IsFacingLeft());
                koopa->Kick(kickLeft);
                m_Session.AddScore(400);
                SpawnScorePopup(400, ePos);
                m_Audio.PlaySFX("kick");
                glm::vec2 resolvedPos = player.GetPosition();
                resolvedPos.y = enemyTop - pSize.y - 0.1f;
                player.SetPosition(resolvedPos);
                glm::vec2 vel = player.GetVelocity();
                vel.y = -180.0f;
                player.SetVelocity(vel);
                LOG_INFO("Koopa shell top-kicked {}!", kickLeft ? "left" : "right");
                return;
            }

            const Enemy::StompOutcome outcome = enemy->Stomp();
            if (outcome == Enemy::StompOutcome::Defeated ||
                outcome == Enemy::StompOutcome::EnteredShell) {
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
            } else if (outcome == Enemy::StompOutcome::LostWings) {
                m_Session.AddScore(kParatroopaDemoteScore);
                SpawnScorePopup(kParatroopaDemoteScore, ePos);
                m_Audio.PlaySFX("stomp");
                LOG_INFO("KoopaParatroopa demoted: score={}", kParatroopaDemoteScore);
            } else if (outcome == Enemy::StompOutcome::StoppedShell) {
                m_Audio.PlaySFX("stomp");
                LOG_INFO("Koopa shell stopped or revive timer reset.");
            }

            if (outcome != Enemy::StompOutcome::NoEffect) {
                glm::vec2 vel = player.GetVelocity();
                vel.y = -180.0f;
                player.SetVelocity(vel);
            }
        } else {
            // ── 側碰 / 從下方衝入 ──
            if (isStationaryShell) {
                // 從側面碰到靜止的龜殼 → 直接踢飛（400 分），且不會有往上的彈跳！
                const float playerCenterX = pPos.x + pSize.x * 0.5f;
                const float shellCenterX = ePos.x + eSize.x * 0.5f;
                const bool kickLeft = Koopa::KickLeftFromContact(
                    playerCenterX, shellCenterX, player.IsFacingLeft());
                koopa->Kick(kickLeft);
                m_Session.AddScore(400);
                SpawnScorePopup(400, ePos);
                m_Audio.PlaySFX("kick"); // 踢殼音效
                LOG_INFO("Koopa shell kicked {} from side!", kickLeft ? "left" : "right");
            } else {
                // 碰到有殺傷力的敵人：玩家受傷降級
                // 變身動畫中視為無敵，不可被重複傷害
                if (!player.IsDamageInvincible() && !player.IsTransforming()) {
                    player.Downgrade();
                    if (player.IsDying()) {
                        // SMALL 直接死亡，無無敵倒數需求
                    } else if (player.IsTransforming()) {
                        // 觸發縮小動畫：無敵倒數等動畫結束後才開始
                        pendingDamageInvincibility = 2.0f;
                        m_Audio.PlaySFX("pipe"); // 受傷縮小音效
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
    for (auto& shellEnemy : m_Enemies) {
        if (!shellEnemy->CanCollide()) continue;

        auto* koopa = dynamic_cast<Koopa*>(shellEnemy.get());
        if (!koopa || !koopa->IsSliding()) continue;  // 只有滑行中的殼才能傷到敵人

        const glm::vec2 sPos  = koopa->GetPosition();
        const glm::vec2 sSize = koopa->GetSize();

        for (auto& target : m_Enemies) {
            if (target.get() == shellEnemy.get()) continue;  // 不撞自己
            if (!target->CanCollide()) continue;

            const glm::vec2 tPos  = target->GetPosition();
            const glm::vec2 tSize = target->GetSize();

            if (CollisionUtils::CheckAABB(sPos, sSize, tPos, tSize)) {
                // 殼命中 Koopa 系列：觸發翻轉死亡
                const bool flipLeft = koopa->GetVelocity().x < 0.0f;
                if (auto* targetKoopa = dynamic_cast<Koopa*>(target.get())) {
                    targetKoopa->Die(flipLeft);
                } else if (auto* targetGoomba = dynamic_cast<Goomba*>(target.get())) {
                    targetGoomba->Die(flipLeft);
                } else {
                    target->SetAlive(false);
                    target->SetVisible(false);
                }
                // 殼連殺使用獨立的殼 combo 計分
                const int pts = ShellComboScore(koopa->ConsumeShellChainIndex());
                if (pts < 0) {
                    m_Session.AddLife();
                    SpawnScorePopup(-1, tPos);
                } else {
                    m_Session.AddScore(pts);
                    SpawnScorePopup(pts, tPos);
                }
                LOG_INFO("Shell killed enemy: combo={} score={} shellPos={} targetPos={}",
                         koopa->GetShellChainCount(), pts, sPos, tPos);
            }
        }
    }
}

// ─── CheckBlockCollision ──────────────────────────────────────────────────
void GameManager::CheckBlockCollision(Player& player) {
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

    const glm::vec2 pPosOrig = player.GetPosition();  // 原始位置，整個函式內不變
    const glm::vec2 pPrev    = player.GetPreviousPosition();
    const glm::vec2 pSize    = player.GetSize();
    glm::vec2 pPos           = pPosOrig;                // 工作用位置，可累積修正
    glm::vec2 pVel           = player.GetVelocity();
    bool onGround = false;

    // 效能優化：複用 member 暫存容器，避免每幀 heap allocation
    m_TmpBlocksToRemove.clear();

    constexpr float SURFACE_TOLERANCE = 2.0f;
    // 頂磚所需的最小水平重疊：與落地的 MIN_SUPPORT_OVERLAP 同理。
    // 若不設門檻，貼著「兩格高的牆」往右戳進次像素（例如 0.1px），
    // 起跳上升時牆上半格的底邊會被誤判成頭頂天花板，把跳躍壓回原地
    // ——表現為「貼牆跳不起來」。要求真正鑽到磚下方才算頂到。
    constexpr float MIN_BONK_OVERLAP = 4.0f;
    // 頂點附近（|vy|→0）只算物理接觸、不觸發 OnHit；仍有明顯上衝速度才視為蓄意撞磚。
    // 不能用「穿透深度」判定：單幀位移上限只有 ~251/60 ≈ 4.2px，比深度門檻還小，
    // 任何頂磚都會被當成擦頂 → OnHit 永遠不觸發。改用上升速度才可靠。
    // 110px/s：配合跳躍初速 255，把大馬利歐的擊碎上限卡在第 6 格。
    // 大馬利歐站地滿跳撞各格的上升速度：第6格≈148(碎)、第7格≈86(不碎)，門檻取中間值。
    // 頂點附近速度趨近 0 → 擦到不觸發（想跳上去站平台、不撞破那層磚）。
    //
    // 小馬利歐矮 16px、跳躍初速相同，碰到同一排磚時頭要多飛 16px，撞擊瞬間的
    // 上升速度因重力多衰減而常落在 110 以下 → 會被誤判成擦頂，導致「小馬利歐
    // 頂不死磚上的敵人」。因此小馬利歐用較低門檻補回這段高度差造成的速度損失。
    const float MIN_BONK_TRIGGER_SPEED =
        (player.GetForm() == Player::Form::SMALL) ? 70.0f : 110.0f;

    // ── Pass 1：找到唯一的 hitFromBelow 候選 ─────────────────────────────
    // 選擇標準：水平重疊量最大的方塊 = 馬力歐「最正對」的那格。
    // 不能用 blockBottom 最大：同一排 ABCD 的 blockBottom 完全相同，
    // 用 blockBottom 只會選到迴圈最後通過的那個，導致打到旁邊的方塊。
    std::shared_ptr<Block> hitBelowBlock = nullptr;
    // 初值設為門檻：只有水平重疊「超過」MIN_BONK_OVERLAP 的方塊才會被選為頂磚，
    // 純貼牆的次像素邊緣接觸（overlap≈0）因此被濾掉，不再誤觸頭頂判定。
    float bestOverlapX = MIN_BONK_OVERLAP;
    std::shared_ptr<Block> ignoredShallowHeadBlock = nullptr;
    float bestIgnoredShallowOverlapX = MIN_BONK_OVERLAP;

    if (pVel.y < 0.0f) { // 只有往上飛時才可能從下方碰
        const float previousTop = pPrev.y;

        for (const auto& block : m_Blocks) {
            // HiddenBlock 揭露前 IsSolid() == false，但往上頂碰時仍需觸發 OnHit（揭露效果）。
            // 因此對未揭露的 HiddenBlock 也要進入 Pass 1 掃描；Pass 2 因 IsSolid() == false
            // 會跳過它，所以揭露前不會阻擋玩家走路或站立。
            const auto* hiddenBlk = dynamic_cast<const HiddenBlock*>(block.get());
            const bool isUnrevealedHidden = hiddenBlk && !hiddenBlk->IsRevealed();
            if (!block->IsSolid() && !isUnrevealedHidden) continue;
            // 單向平台：從下方上升直接穿過，不觸發頂磚
            if (block->IsOneWay()) continue;

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
                const float bonkDepth = blockBottom - pPosOrig.y;

                if (-pVel.y >= MIN_BONK_TRIGGER_SPEED && overlapX > bestOverlapX) {
                    bestOverlapX = overlapX;
                    hitBelowBlock = block;
                } else if (block->IsSolid() && bonkDepth > 0.0f && overlapX > bestIgnoredShallowOverlapX) {
                    bestIgnoredShallowOverlapX = overlapX;
                    ignoredShallowHeadBlock = block;
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

        BlockHitResult hitRes = hitBelowBlock->OnHit(&player);

        if (hitRes.isDestroyed) {
            SpawnBrickDebris(bPos);
            hitBelowBlock->SetVisible(false);
            m_TmpBlocksToRemove.push_back(hitBelowBlock);
            // NES 原版打碎磚塊會加 50 分，但不顯示浮動分數。
            m_Session.AddScore(50);
            m_Audio.PlaySFX("brick_break"); // 磚塊破碎音效
        }

        if (hitRes.spawnItem != "None") {
            SpawnItem(hitRes.spawnItem, bPos, &player);
        } else if (!hitRes.isDestroyed) {
            // 頂到方塊但沒有打破也沒有道具：播 bump 音效（頂空磚或不可破磚）
            m_Audio.PlaySFX("bump");
        }

        // ── 頂磚效果：消滅方塊正上方的敵人，踢飛正上方的道具 ──────────
        // 仿原版：只有「會被頂出反應」的磚（磚塊、問號磚…）才會波及上方；
        // 純地形（樹台、地面、水管…）只是擋住頭，不殺敵也不彈道具。
        // 判定範圍：敵人/道具底部距方塊頂部 ±BUMP_ABOVE_TOL 像素內，
        // 且水平中心在方塊左右邊界以內（含半格緩衝）。
        if (hitBelowBlock->BumpsContentsAbove()) {
        constexpr float BUMP_ABOVE_TOL  = 4.0f;   // 距方塊頂部的垂直容忍
        constexpr float BUMP_ITEM_KICK  = -200.0f; // 道具被踢飛的上拋初速

        const float blockTopY   = bPos.y;
        const float blockLeft   = bPos.x;
        const float blockRight  = bPos.x + bSize.x;

        for (auto& enemy : m_Enemies) {
            if (!enemy->CanCollide()) continue;

            const glm::vec2 ePos  = enemy->GetPosition();
            const glm::vec2 eSize = enemy->GetSize();

            const float enemyBottom  = ePos.y + eSize.y;
            const float enemyCenterX = ePos.x + eSize.x * 0.5f;

            // 底部接近方塊頂面，且水平有重疊
            if (std::abs(enemyBottom - blockTopY) <= BUMP_ABOVE_TOL &&
                enemyCenterX >= blockLeft && enemyCenterX <= blockRight) {
                const bool flipLeft = enemyCenterX < (bPos.x + bSize.x * 0.5f);
                if (auto* koopa = dynamic_cast<Koopa*>(enemy.get())) {
                    koopa->Die(flipLeft);
                } else if (auto* goomba = dynamic_cast<Goomba*>(enemy.get())) {
                    goomba->Die(flipLeft);  // 仿原版：翻倒飛出，而非踩扁
                } else {
                    enemy->Stomp();
                }
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
        } // BumpsContentsAbove()
    }

    // ── Pass 2A：只用頂面接觸判斷落地 ───────────────────────────────────
    // 落地與側碰分開處理，避免左右牆壁的穿透被誤判為站在地面上。
    constexpr float SUPPORT_TOLERANCE = 0.5f;
    // 最小支撐重疊：原版 NES 的腳是內縮約 2px 的取樣點，
    // 不足此值視為沒踩到——防止「次像素腳尖」掛在方塊邊緣懸空站立，
    // 也削弱貼牆下落時把牆磚頂邊誤判為地板的貼牆跳。
    constexpr float MIN_SUPPORT_OVERLAP = 3.0f;
    const float previousBottom = pPrev.y + pSize.y;
    const float currentBottom = pPos.y + pSize.y;
    std::shared_ptr<Block> landingBlock = nullptr;
    float landingTop = std::numeric_limits<float>::max();

    for (const auto& block : m_Blocks) {
        if (!block->IsSolid()) continue;
        if (block == hitBelowBlock) continue;

        const glm::vec2 bPos = block->GetPosition();
        const glm::vec2 bSize = block->GetSize();
        const float overlapLeft = std::max(pPos.x, bPos.x);
        const float overlapRight = std::min(pPos.x + pSize.x, bPos.x + bSize.x);
        const float overlapX = overlapRight - overlapLeft;
        if (overlapX < MIN_SUPPORT_OVERLAP) continue;

        // 移動平台：它在本幀已先移動（見更新順序），其頂面每幀最多位移
        // |frameDelta.y|。若沿用固定 0.5px 容差，當平台上升速度 > 容差時，
        // 頂面會從玩家腳底溜走、判不到支撐 → 乘客被丟包墜落。
        // 因此對移動平台把容差加上本幀垂直位移量，維持站立接觸。
        float supportTolerance = SUPPORT_TOLERANCE;
        auto* movingPlatform = dynamic_cast<MovingPlatformBlock*>(block.get());
        if (movingPlatform) {
            supportTolerance += std::abs(movingPlatform->GetFrameDelta().y);
        }

        const bool restingOnSurface =
            pVel.y >= 0.0f &&
            std::abs(currentBottom - bPos.y) <= supportTolerance;
        const bool crossedSurface =
            pVel.y >= 0.0f &&
            previousBottom <= bPos.y &&
            currentBottom >= bPos.y;

        // 診斷：玩家在移動平台頂面附近（24px 內）時記錄支撐判定數據，
        // 只在靠近時印出避免洗版，用來抓玩家從平台脫離的那一幀。
        if (movingPlatform && std::abs(currentBottom - bPos.y) < 24.0f) {
            const glm::vec2 fd = movingPlatform->GetFrameDelta();
            LOG_DEBUG(
                "[MovPlat] plat=({:.2f},{:.2f}) frameDelta=({:.3f},{:.3f}) "
                "pBottom={:.2f} prevBottom={:.2f} gap={:.3f} tol={:.3f} "
                "overlapX={:.2f} pVelY={:.2f} resting={} crossed={}",
                bPos.x, bPos.y, fd.x, fd.y,
                currentBottom, previousBottom, currentBottom - bPos.y,
                supportTolerance, overlapX, pVel.y, restingOnSurface, crossedSurface);
        }

        // 當一幀跨過多個頂面時，選下降路徑中最先碰到的最高表面。
        if ((restingOnSurface || crossedSurface) && bPos.y < landingTop) {
            landingBlock = block;
            landingTop = bPos.y;
        }
    }

    if (landingBlock) {
        pPos.y = landingTop - pSize.y;
        if (pVel.y > 0.0f) pVel.y = 0.0f;
        onGround = true;

        if (auto* platform = dynamic_cast<MovingPlatformBlock*>(landingBlock.get())) {
            const glm::vec2 fd = platform->GetFrameDelta();
            // 只跟隨水平位移；垂直方向已由上面「貼齊頂面」自動完成。
            // 不可再 += fd.y，否則會把玩家額外往下推一幀份（領先平台），
            // 在平台到行程端點減速/反折時造成 ~frameDelta 的間隙而脫離墜落。
            pPos.x += fd.x;
            LOG_DEBUG("[MovPlat] CARRY: carried by platform=({:.2f},{:.2f}) "
                      "frameDelta=({:.3f},{:.3f}) -> playerPos=({:.2f},{:.2f})",
                      landingBlock->GetPosition().x, landingBlock->GetPosition().y,
                      fd.x, fd.y, pPos.x, pPos.y);
        }
    }

    // ── Pass 2B：剩餘重疊一律視為水平側碰 ───────────────────────────────
    // 側碰只修正 X，不得停止下落或把玩家設為 grounded。
    bool touchingWallLeft  = false; // 牆在玩家左邊（玩家被往右推）
    bool touchingWallRight = false; // 牆在玩家右邊（玩家被往左推）
    for (const auto& block : m_Blocks) {
        if (!block->IsSolid()) continue;
        if (block == hitBelowBlock) continue;
        if (block == ignoredShallowHeadBlock) continue;
        // 單向平台：側面接觸直接穿過，不側推（只能從上方落地，見 Pass 2A）
        if (block->IsOneWay()) continue;

        const glm::vec2 bPos = block->GetPosition();
        const glm::vec2 bSize = block->GetSize();
        if (!CollisionUtils::CheckAABB(pPos, pSize, bPos, bSize)) continue;

        const float blockBottom = bPos.y + bSize.y;
        const bool ceilingOverlap =
            pPrev.y >= blockBottom - SURFACE_TOLERANCE &&
            pPos.y < blockBottom;
        if (ceilingOverlap) continue;

        const float pCenterX = pPos.x + pSize.x * 0.5f;
        const float pCenterY = pPos.y + pSize.y * 0.5f;
        const float bCenterX = bPos.x + bSize.x * 0.5f;
        const float bCenterY = bPos.y + bSize.y * 0.5f;
        const float dx = pCenterX - bCenterX;
        const float dy = pCenterY - bCenterY;
        const float penX = (pSize.x * 0.5f + bSize.x * 0.5f) - std::abs(dx);
        const float penY = (pSize.y * 0.5f + bSize.y * 0.5f) - std::abs(dy);
        if (penY <= penX) continue;

        if (dx > 0.0f) { pPos.x += penX; touchingWallLeft  = true; }
        else           { pPos.x -= penX; touchingWallRight = true; }
    }

    // 貼牆後把「撞進牆裡」的水平速度歸零：座標已被夾在牆面（座標＝貼牆），
    // 速度不再每幀累積，消除「位移→反彈」的抖動，也讓貼牆時起跳/物理乾淨。
    // 只歸零指向牆的分量——反方向（離牆）仍可正常加速，符合「除非反方向」。
    if (touchingWallLeft  && pVel.x < 0.0f) pVel.x = 0.0f;
    if (touchingWallRight && pVel.x > 0.0f) pVel.x = 0.0f;

    player.SetPosition(pPos);
    player.SetVelocity(pVel);
    player.SetOnGround(onGround);

    // 清理碎裂的方塊（使用 member 暫存容器）
    for (auto& b : m_TmpBlocksToRemove) {
        m_Blocks.erase(std::remove(m_Blocks.begin(), m_Blocks.end(), b), m_Blocks.end());
    }
}


void GameManager::CheckEnemyBlockCollision() {
    for (auto& enemy : m_Enemies) {
        if (!enemy->CanCollide()) continue;
        if (!enemy->UsesBlockCollision()) continue;

        glm::vec2 ePos  = enemy->GetPosition();
        glm::vec2 eSize = enemy->GetSize();
        glm::vec2 eVel  = enemy->GetVelocity();
        bool grounded = false;

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
                    if (eVel.y > 0) {
                        eVel.y = 0.0f;
                        grounded = true;
                    }
                }
            }
        }

        if (auto* koopa = dynamic_cast<Koopa*>(enemy.get())) {
            if (koopa->GetVariant() == Koopa::Variant::Red &&
                koopa->IsWalking() &&
                grounded) {
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

                if (!hasSupportAhead) {
                    enemy->ReverseDirection();
                    eVel = enemy->GetVelocity();
                }
            }
        }

        enemy->SetPosition(ePos);
        enemy->SetVelocity(eVel);
        enemy->SetGrounded(grounded);
    }
}

void GameManager::CheckEnemyEnemyCollision() {
    for (std::size_t i = 0; i < m_Enemies.size(); ++i) {
        auto& first = m_Enemies[i];
        if (!first->CanCollide()) continue;
        if (!first->UsesBlockCollision()) continue;

        auto* firstKoopa = dynamic_cast<Koopa*>(first.get());
        if (firstKoopa && firstKoopa->IsSliding()) continue;

        for (std::size_t j = i + 1; j < m_Enemies.size(); ++j) {
            auto& second = m_Enemies[j];
            if (!second->CanCollide()) continue;
            if (!second->UsesBlockCollision()) continue;

            auto* secondKoopa = dynamic_cast<Koopa*>(second.get());
            if (secondKoopa && secondKoopa->IsSliding()) continue;

            glm::vec2 firstPos = first->GetPosition();
            glm::vec2 firstVel = first->GetVelocity();
            const glm::vec2 firstSize = first->GetSize();

            glm::vec2 secondPos = second->GetPosition();
            glm::vec2 secondVel = second->GetVelocity();
            const glm::vec2 secondSize = second->GetSize();

            if (!CollisionUtils::CheckAABB(firstPos, firstSize, secondPos, secondSize)) continue;

            const float firstCenterX = firstPos.x + firstSize.x * 0.5f;
            const float firstCenterY = firstPos.y + firstSize.y * 0.5f;
            const float secondCenterX = secondPos.x + secondSize.x * 0.5f;
            const float secondCenterY = secondPos.y + secondSize.y * 0.5f;

            const float dx = firstCenterX - secondCenterX;
            const float dy = firstCenterY - secondCenterY;

            const float penX = (firstSize.x + secondSize.x) * 0.5f - std::abs(dx);
            const float penY = (firstSize.y + secondSize.y) * 0.5f - std::abs(dy);

            if (penX <= 0.0f || penY <= 0.0f) continue;
            if (penX > penY) continue;

            const bool firstMoving = std::abs(firstVel.x) > 1.0f;
            const bool secondMoving = std::abs(secondVel.x) > 1.0f;
            if (!firstMoving && !secondMoving) continue;

            const bool firstOnLeft = dx <= 0.0f;
            float firstPush = penX * 0.5f;
            float secondPush = penX * 0.5f;
            if (firstMoving && !secondMoving) {
                firstPush = penX;
                secondPush = 0.0f;
            } else if (!firstMoving && secondMoving) {
                firstPush = 0.0f;
                secondPush = penX;
            }

            if (firstOnLeft) {
                firstPos.x -= firstPush;
                secondPos.x += secondPush;
            } else {
                firstPos.x += firstPush;
                secondPos.x -= secondPush;
            }

            first->SetPosition(firstPos);
            second->SetPosition(secondPos);

            if (firstMoving) {
                first->ReverseDirection();
            }
            if (secondMoving) {
                second->ReverseDirection();
            }
        }
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
void GameManager::SpawnItem(const std::string& itemType, glm::vec2 position, Player* sourcePlayer) {
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
        const Player::Form sourceForm = sourcePlayer ? sourcePlayer->GetForm() : m_Player.GetForm();
        if (sourceForm == Player::Form::SMALL) {
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
    // 效能優化：複用 member 暫存容器
    m_TmpItemsToRemove.clear();

    for (auto& item : m_Items) {
        // 香菇只有在 Active 狀態才能吃
        // 金幣不透過接觸吃，但金幣生命週期結束變成 Collected 也會移除
        if (item->GetState() == ItemState::Active && item->GetType() != "Coin") {
            const glm::vec2 iPos  = item->GetPosition();
            const glm::vec2 iSize = item->GetSize();

            Player* collectingPlayer = nullptr;
            if (m_Player.IsAlive()) {
                const glm::vec2 pPos  = m_Player.GetPosition();
                const glm::vec2 pSize = m_Player.GetSize();
                if (CollisionUtils::CheckAABB(pPos, pSize, iPos, iSize)) {
                    collectingPlayer = &m_Player;
                }
            }

            if (collectingPlayer) {
                const std::string itemType = item->GetType();
                item->OnCollect(collectingPlayer);
                if (item->GetState() == ItemState::Collected) {
                    LOG_INFO("Item collected: type='{}' position={}", itemType, iPos);
                    if (itemType == "LevelCoin") {
                        m_Session.AddCoin();
                        m_Session.AddScore(200);
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
                        m_Audio.PlayEventBGM(kStarmanBGMName);
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
            if (!enemy->CanCollide()) continue;

            // 食人花隱藏中不受火球傷害（由 PiranhaPlant::IsHidden() 保護）
            auto* piranha = dynamic_cast<PiranhaPlant*>(enemy.get());
            if (piranha && piranha->IsHidden()) continue;

            if (CollisionUtils::CheckAABB(fPos, fSize, enemy->GetPosition(), enemy->GetSize())) {
                const glm::vec2 ePos = enemy->GetPosition();
                // Koopa 系列（含飛行龜）：火球直接觸發翻轉死亡，得 200 分；其他敵人（Goomba 等）得 100 分
                // 飛行龜的 Die() 會自動掉翅膀再翻肚（見 KoopaParatroopa::Die）。
                auto* koopa = dynamic_cast<Koopa*>(enemy.get());
                const bool isKoopa = (koopa != nullptr);
                const bool flipLeft = fVel.x < 0.0f; // 火球飛行方向決定翻轉朝向
                if (koopa) {
                    koopa->Die(flipLeft);
                } else if (auto* goomba = dynamic_cast<Goomba*>(enemy.get())) {
                    goomba->Die(flipLeft);
                } else {
                    enemy->SetAlive(false);
                    enemy->SetVisible(false);
                }
                const int pts = isKoopa ? 200 : 100;
                m_Session.AddScore(pts);
                SpawnScorePopup(pts, ePos);
                m_Audio.PlaySFX("kick");
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
void GameManager::UpdateCheckpoints(const Player& player) {
    if (m_Level.checkpoints.empty()) return;

    const float playerX = player.GetPosition().x;

    for (const auto& cp : m_Level.checkpoints) {
        // 玩家中心 X 越過中繼點 X 才算達成
        if (playerX >= cp.x) {
            if (!m_LastCheckpoint.has_value() ||
                cp.x > m_LastCheckpoint->x) {
                m_LastCheckpoint = cp;
                m_Session.CurrentPlayer().checkpoint = cp;
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
void GameManager::CheckFlagCollision(Player& player) {
    if (!player.IsAlive())  return;
    if (m_LevelCleared)       return; // 已過關，不重複計算

    const glm::vec2 pPos  = player.GetPosition();
    const glm::vec2 pSize = player.GetSize();

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
        SpawnScorePopup(score, {fPos.x + fSize.x + 8.0f, pPos.y});
        m_LevelCleared = true;
        m_LevelClearPlayer = &player;

        // 記下旗杆底 Y（= 地面），稍後城堡升旗用作基準高度
        m_FlagBottomY = fPos.y + fSize.y;
        m_LevelClearCastleFlagBaseTiles = flagBlock->GetCastleFlagBaseTiles();

        // 觸發旗子下降動畫（與玩家下滑同速、同時進行）
        flagBlock->StartDescent();

        // 觸發玩家的過關下降演出
        player.StartLevelClearSequence(fPos.x, fPos.y + fSize.y, flagBlock->GetClearWalkTiles());

        // 碰到旗杆：播放旗杆 SFX，同時停掉關卡 BGM
        m_Audio.StopBGM();
        m_Audio.PlaySFX("flagpole");
        LOG_INFO("=== LEVEL CLEAR! Flag score: {}  Total score: {} ===",
                 score,
                 m_Session.CurrentPlayer().score);
        break;
    }
}
