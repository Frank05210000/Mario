#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <memory>
#include <optional>
#include <string>
#include <array>
#include <vector>

#include "Block.hpp"
#include "BrickDebris.hpp"
#include "BrickBlock.hpp"
#include "Camera.hpp"
#include "FlagBlock.hpp"
#include "Goomba.hpp"
#include "GroundBlock.hpp"
#include "GameSession.hpp"
#include "HiddenBlock.hpp"
#include "Koopa.hpp"
#include "KoopaParatroopa.hpp"
#include "LevelData.hpp"
#include "LevelCoinItem.hpp"
#include "MovingPlatformBlock.hpp"
#include "MultiCoinBlock.hpp"
#include "OneUpMushroomItem.hpp"
#include "PipeBlock.hpp"
#include "PiranhaPlant.hpp"
#include "Player.hpp"
#include "Enemy.hpp"
#include "Block.hpp"
#include "Item.hpp"
#include "QuestionBlock.hpp"
#include "StarmanItem.hpp"
#include "TreePlatformBlock.hpp"
#include "UsedOnHitBrickBlock.hpp"
#include "WallBlock.hpp"
#include "Fireball.hpp"
#include "HUD.hpp"
#include "AudioManager.hpp"
#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"
#include "Util/Text.hpp"

/*
 * GameManager：遊戲管理器
 *
 * 持有所有遊戲物件（Player、Enemy、Block），每幀負責：
 *   1. 呼叫所有角色的 Update()（更新邏輯）
 *   2. 更新鏡頭位置
 *   3. 判斷碰撞（誰踩到誰）
 *   4. 呼叫所有角色的 Draw()（同步畫面位置）
 *   5. 呼叫 Renderer 渲染到畫面上
 *
 * OOP 關係：
 *   - GameManager「持有」Player、Enemy、Block、Camera（組合，has-a）
 *   - GameManager 不繼承任何人（它是管理者，不是角色）
 */
class GameManager {
public:
    /* 遊戲開始時呼叫一次
     * 從 JSON 讀入關卡、建立 Enemy、設定玩家出生點、建立渲染器。
     */
    void Start();

    /* 每幀呼叫一次
     * 依序：更新所有角色 → 更新鏡頭 → 碰撞判斷 → 同步畫面 → 渲染。
     */
    void Update();

private:
    enum class FlowState {
        Title,
        LevelIntro,
        IntroCutscene,
        Playing,
        TimeUp,
        LevelClearTransition,  // 時間結算倒數（每幀扣時間、加分）
        LevelClearPause,       // 結算完後停頓約 1 秒，再進下一關 intro
        GameOver,              // 命數歸零後顯示 GAME OVER 畫面約 3 秒再回標題
    };

    /* 讀入關卡 JSON，建立背景圖與所有方塊
     * 同時從 SpawnPoint 物件設定玩家出生點。
     */
    void LoadLevel(const std::string& jsonPath);
    void ChangeLevel(const std::string& levelName, std::optional<glm::vec2> spawnOverride = std::nullopt);
    void ResetSceneObjects();
    void StartNewGame();
    bool HandleLevelCheatShortcut();
    void EnterTitleScreen();
    void EnterLevelIntro();
    void EnterPlaying();
    void EnterTimeUp();
    void EnterGameOver();
    void EnterLevelClearTransition();   // 進入時間結算倒數狀態
    void EnterLevelClearPause();        // 進入結算後停頓狀態
    void AdvanceToNextLevel();          // 推進到下一關（或回標題）
    void BuildTitleOverlay();
    void BuildLevelIntroOverlay();
    void BuildTimeUpOverlay();
    void BuildGameOverOverlay();
    void AddOverlayText(const std::string& text, int fontSize, glm::vec2 position, float zIndex = 30.0f);
    void AddOverlayImage(const std::string& assetPath, glm::vec2 position, glm::vec2 scale, float zIndex = 30.0f);
    std::array<Player*, 2> Players();
    std::array<const Player*, 2> Players() const;
    void ConfigurePlayers();
    void ResetPlayersForNewGame();
    void SetPlayersSpawnPosition(glm::vec2 position);
    void UpdateCameraForPlayers();
    bool AnyPlayerAlive() const;
    bool AnyPlayerDying() const;
    bool AnyPlayerTransforming() const;
    bool AnyPlayerLevelClearFinished() const;
    Player& LeadPlayer();
    const Player& LeadPlayer() const;
    float GetClosestAlivePlayerX(glm::vec2 position) const;
    void UpdateTitle(float dt);
    void UpdateLevelIntro(float dt);
    void UpdateIntroCutscene(float dt);
    void UpdatePlaying(float dt);
    void UpdateTimeUp(float dt);
    void UpdateGameOver(float dt);
    void UpdateLevelClearTransition(float dt);  // 時間結算倒數
    void UpdateLevelClearPause(float dt);        // 結算後停頓
    void DrawScene(bool updateHud);
    bool CheckPipeTransition(Player& player);
    bool TryEnterIntroCutscene();
    PipeBlock* FindIntroCutscenePipe() const;
    static std::string MakeLevelPath(const std::string& levelName);

    // 處理玩家與方塊、地形的物理碰撞
    void CheckBlockCollision(Player& player);

    // 處理敵人與方塊、地形的物理碰撞
    void CheckEnemyBlockCollision();

    // 負責統籌與產生物件
    void SpawnItem(const std::string& itemType, glm::vec2 position, Player* sourcePlayer = nullptr);
    void CheckItemCollision();
    void CheckItemBlockCollision();

    /*
     * 鏡頭觸發敵人生成
     * 每幀掃描 m_EnemySpawnQueue，當敵人的世界 X 座標進入鏡頭右邊界時才建立物件。
     */
    void SpawnEnemy(const ObjectData& data);
    void CheckEnemySpawnQueue();

    void SpawnFireball(glm::vec2 position, bool movingLeft);
    void CheckFireballCollision();
    void SpawnBrickDebris(glm::vec2 position);
    void SavePlayerProgress();
    void ApplyPlayerProgress();
    void HandleLifeLost();

    /* 踩踏碰撞判斷
     * 檢查 Player 是否從上方踩到任何 Enemy，並依 StompOutcome
     * 處理縮殼、停殼、掉翅膀、得分與反彈。
     */
    void CheckStompCollision(Player& player, float& pendingDamageInvincibility);

    /* 殼對敵人碰撞判斷
     * 檢查正在滑行中的 Koopa 殼是否撞到其他存活的敵人。
     * 若重疊則直接消滅被撞的敵人；連殺進度由各滑行殼自行持有。
     */
    void CheckShellEnemyCollision();

    /* 終點旗子碰撞
     * 偵測玩家是否碰到旗杆，若是計算得分並觸發過關。
     */
    void CheckFlagCollision(Player& player);

    /* 建立渲染場景
     * 把所有要顯示的物件加入 m_Renderer。
     */
    void BuildScene();

    // ─── 持有的物件 ────────────────────────────────────────────────

    Player                              m_Player;
    Player                              m_Player2;
    std::vector<std::shared_ptr<Enemy>> m_Enemies;           // 已生成、存活中的敵人
    std::vector<ObjectData>             m_EnemySpawnQueue;   // 還沒生成的敵人（等鏡頭到達）
    // 道具
    std::vector<std::shared_ptr<Item>> m_Items;    
    std::vector<std::shared_ptr<Block>>  m_Blocks;     // 所有靜態場景物件
    std::vector<std::shared_ptr<Fireball>> m_Fireballs;
    std::vector<std::shared_ptr<BrickDebris>> m_BrickDebris;

    Camera      m_Camera;                              // 鏡頭（追蹤玩家，計算捲動偏移）
    LevelData   m_Level;                               // 關卡資料（供 Camera 取得 levelWidth）
    ThemeAssets m_ThemeAssets;                         // 當前關卡主題，隨 LoadLevel 更新

    std::shared_ptr<Util::GameObject>    m_Background; // 背景圖（zIndex=0，畫在最底層）

    Util::Renderer m_Renderer;                         // PTSD 渲染器，負責把物件畫到螢幕上

    GameSession m_Session;
    FlowState m_FlowState = FlowState::Title;
    float m_StateTimer = 0.0f;
    float m_LevelClearTransitionTimer = 0.0f;
    std::vector<std::shared_ptr<Util::GameObject>> m_OverlayObjects;

    bool  m_LevelCleared = false;          // 是否已觸碰旗杆（防止重複計分）
    bool  m_WaitingForTimeUpDeath = false; // 時間到後等待玩家死亡動畫播完再進 TimeUp
    bool  m_PlayerWasDying = false;        // 上幀是否已在死亡狀態（偵測死亡事件用）
    float m_DeathSequenceTimer = 0.0f;     // 死亡 BGM / 死亡動畫流程同步計時
    bool  m_HurryUpTriggered = false;       // 剩餘時間低於 100 後只切換一次 hurry-up BGM
    bool  m_Paused = false;                // 暫停狀態（只在 Playing 下有效）
    std::shared_ptr<Util::GameObject> m_PauseOverlay; // 「PAUSED」文字 overlay
    // 受傷縮小變身動畫結束後需要啟動的傷害無敵計時（秒）；0 = 無待定
    float m_PendingDamageInvincibility = 0.0f;
    float m_PendingDamageInvincibility2 = 0.0f;
    std::string m_SelectedInitialLevelName = "1-1"; // 標題選關用（不影響遊戲中推進）
    std::string m_SelectedWorldLabel = "1-1";

    // ─── 關卡鏈（Batch A）─────────────────────────────────────────────────
    // 每個元素：{levelName, worldLabel}
    // 最後一關過關後 AdvanceToNextLevel() 回標題
    struct LevelEntry { std::string levelName; std::string worldLabel; };
    static const std::vector<LevelEntry> kLevelChain;

    // 目前正在遊玩的關卡在 kLevelChain 中的 index
    // -1 代表尚未進入遊戲（標題畫面）
    int m_CurrentLevelIndex = -1;

    // 時間結算：每幀扣減剩餘時間並同步加分
    // 用與 m_TimeRemaining 相同的欄位，但每幀一次扣一批
    float m_CountdownAccum = 0.0f; // 累積時間，每秒扣一單位（NES 以幀率計算）

    // ─── 水管進出動畫緩衝變數 ───────────────────────────────────────────
    std::string m_PendingLevel;
    std::optional<glm::vec2> m_PendingSpawn;

    // ─── 中繼點系統 ───────────────────────────────────────────────────
    // 目前存活期間玩家已越過的最後一個中繼點（nullopt 表示尚未達成任何中繼點）
    // 換關時必須清空，確保死亡重生位置對應到正確關卡
    std::optional<glm::vec2> m_LastCheckpoint;
    // 暫存「本次重生要用的中繼點座標」，由 HandleLifeLost 設定，
    // EnterLevelIntro/EnterPlaying 在 LoadLevel 後讀取並覆蓋 playerSpawn
    std::optional<glm::vec2> m_CheckpointRespawnOverride;
    void UpdateCheckpoints(const Player& player);   // 每幀在 UpdatePlaying 中呼叫，偵測玩家是否越過新中繼點

    // ─── 音效管理器 ────────────────────────────────────────────────
    AudioManager m_Audio;           // BGM / SFX 統一管理
    std::string  m_LevelBGMName;    // 目前關卡的 BGM 名稱（星星結束後恢復用）

    // ─── HUD ──────────────────────────────────────────────────────
    HUD   m_Hud;                  // 遊戲狀態列
    float m_TimeRemaining = 400.0f; // 關卡倒數時間（秒）

    // ─── 效能優化：複用 per-frame 暫存容器，避免每幀 heap alloc ──────
    std::vector<std::shared_ptr<Block>>    m_TmpBlocksToRemove;
    std::vector<std::shared_ptr<Item>>     m_TmpItemsToRemove;
    std::vector<std::shared_ptr<Fireball>> m_TmpFireballsToRemove;

    // ─── Combo 計分 ────────────────────────────────────────────────
    // 踩踏連殺（單次跳躍）
    int m_ComboCount = 0;   // 當前連殺次數（0 = 尚未觸發）
    // 根據 NES Mario 規則返回下一擊的分數，並推進 combo 計數
    int NextComboScore();

    // 重置踩踏 combo（落地時呼叫）
    void ResetCombo();

    // ─── 浮動得分彈出 ─────────────────────────────────────────────
    struct ScorePopup {
        std::shared_ptr<Util::GameObject> obj;
        glm::vec2 worldPos = {0.0f, 0.0f}; // 用世界座標追蹤，每幀再換算螢幕座標
        float timer = 0.0f;    // 已存活秒數
        float totalLife = 0.8f; // 最大存活秒數
    };
    std::vector<ScorePopup> m_ScorePopups;
    void SpawnScorePopup(int score, glm::vec2 worldPos);
    void UpdateScorePopups(float dt);

    // ─── 過關城堡升旗演出 ─────────────────────────────────────────
    // 馬力歐進城門後，城堡頂升起小旗（與時間結算同時進行）。
    // 比照 ScorePopup：以世界座標追蹤，每幀換算螢幕座標。
    std::shared_ptr<Util::GameObject> m_CastleFlag;
    glm::vec2 m_CastleFlagWorldPos = {0.0f, 0.0f};
    float     m_CastleFlagTargetY  = 0.0f;   // 升到頂時的世界 Y
    float     m_FlagBottomY        = 0.0f;   // 碰旗時記下的旗杆底 Y（=地面，城堡基準）
    void SpawnCastleFlag(float doorCenterX, float groundY); // 生成並開始升旗
    void UpdateCastleFlagRaise(float dt);                   // 每幀讓小旗往上升
};

#endif
