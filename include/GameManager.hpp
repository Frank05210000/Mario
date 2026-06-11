#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include <memory>
#include <optional>
#include <string>
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
        Playing,
        TimeUp,
        LevelClearTransition,
    };

    /* 讀入關卡 JSON，建立背景圖與所有方塊
     * 同時從 SpawnPoint 物件設定玩家出生點。
     */
    void LoadLevel(const std::string& jsonPath);
    void ChangeLevel(const std::string& levelName, std::optional<glm::vec2> spawnOverride = std::nullopt);
    void ResetSceneObjects();
    void StartNewGame();
    void EnterTitleScreen();
    void EnterLevelIntro();
    void EnterPlaying();
    void EnterTimeUp();
    void EnterLevelClearTransition();
    void SelectInitialLevel(const std::string& levelName, const std::string& worldLabel);
    void BuildTitleOverlay();
    void BuildLevelIntroOverlay();
    void BuildTimeUpOverlay();
    void BuildLevelClearOverlay();
    void AddOverlayText(const std::string& text, int fontSize, glm::vec2 position, float zIndex = 30.0f);
    void AddOverlayImage(const std::string& assetPath, glm::vec2 position, glm::vec2 scale, float zIndex = 30.0f);
    void UpdateTitle(float dt);
    void UpdateLevelIntro(float dt);
    void UpdatePlaying(float dt);
    void UpdateTimeUp(float dt);
    void UpdateLevelClearTransition(float dt);
    void DrawScene(bool updateHud);
    bool CheckPipeTransition();
    static std::string MakeLevelPath(const std::string& levelName);

    // 處理玩家與方塊、地形的物理碰撞
    void CheckBlockCollision();

    // 處理敵人與方塊、地形的物理碰撞
    void CheckEnemyBlockCollision();

    // 負責統籌與產生物件
    void SpawnItem(const std::string& itemType, glm::vec2 position);
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
     * 檢查 Player 是否從上方踩到任何 Enemy。
     * 若是，呼叫 enemy.Stomp() 讓敵人消失。
     */
    void CheckStompCollision();

    /* 終點旗子碰撞
     * 偵測玩家是否碰到旗杆，若是計算得分並觸發過關。
     */
    void CheckFlagCollision();

    /* 建立渲染場景
     * 把所有要顯示的物件加入 m_Renderer。
     */
    void BuildScene();

    // ─── 持有的物件 ────────────────────────────────────────────────

    Player                               m_Player;     
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

    bool  m_LevelCleared = false; // 是否已觸碰旗杆（防止重複計分）
    std::string m_SelectedInitialLevelName = "1-1";
    std::string m_SelectedWorldLabel = "1-1";

    // ─── 水管進出動畫緩衝變數 ───────────────────────────────────────────
    std::string m_PendingLevel;
    std::optional<glm::vec2> m_PendingSpawn;

    // ─── HUD ──────────────────────────────────────────────────────
    HUD   m_Hud;                  // 遊戲狀態列
    float m_TimeRemaining = 400.0f; // 關卡倒數時間（秒）

    // ─── 效能優化：複用 per-frame 暫存容器，避免每幀 heap alloc ──────
    std::vector<std::shared_ptr<Block>>    m_TmpBlocksToRemove;
    std::vector<std::shared_ptr<Item>>     m_TmpItemsToRemove;
    std::vector<std::shared_ptr<Fireball>> m_TmpFireballsToRemove;
};

#endif
