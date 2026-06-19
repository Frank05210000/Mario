#include "HUD.hpp"

#include <array>
#include <iomanip>
#include <sstream>

#include "AssetPath.hpp"
#include "Util/Logger.hpp"
#include "Util/Renderer.hpp"
#include "config.hpp"   // WINDOW_WIDTH, WINDOW_HEIGHT

// ─── 字體路徑 ──────────────────────────────────────────────────────────────
static const std::string kFontPath =
    MakeAssetPath("font/Super Mario Bros. NES.ttf");

// ─── 字體大小（像素） ──────────────────────────────────────────────────────
// 原版 NES 字元為 8×8 像素，乘 GAME_SCALE(3) = 24 螢幕像素
static constexpr int   kFontSize   = 24;

// ─── 文字顏色（原版 NES：純白）────────────────────────────────────────────
static const Util::Color kWhite = Util::Color(255, 255, 255);

// ─── 螢幕佈局常數（PTSD 座標：原點在畫面中心，Y 向上為正）────────────────
// 視窗尺寸由 PTSD/config.hpp 定義；目前 960×720，半寬＝480，半高＝360。
// 可視世界寬 = 960/3 = 320px，比 NES 的 256px 多 64px，
// 故把 NES 版面水平置中：screen_x = (nes_x + 32) * 3 - 480。
// 原版 NES 版面（NES 像素座標，取各欄位中心）：
//   MARIO/分數欄：x 24..72   → 中心 48
//   金幣圖示    ：x 88..96   → 中心 92；「x00」x 96..120 → 中心 108
//   WORLD/1-1 欄：x 144..184 → 中心 164
//   TIME/400 欄 ：x 200..232 → 中心 216
// 標題列 NES y=16、數值列 y=24，兩行垂直中心 y=24 → 螢幕 y = 360 - 24*3 = 288。
static constexpr float kHalfW  = WINDOW_WIDTH  / 2.0f;
static constexpr float kHalfH  = WINDOW_HEIGHT / 2.0f;
static constexpr float kNesMarginX = (WINDOW_WIDTH / 3.0f - 256.0f) * 0.5f; // = 32
static constexpr float NesX(float nesX) { return (nesX + kNesMarginX) * 3.0f - kHalfW; }

static constexpr float kScoreX    = NesX(48.0f);
static constexpr float kCoinIconX = NesX(92.0f);
static constexpr float kCoinX     = NesX(108.0f);
static constexpr float kWorldX    = NesX(164.0f);
static constexpr float kTimeX     = NesX(216.0f);
static constexpr float kTopY      = kHalfH - 72.0f;

static const std::array<std::string, 3> kCoinFramePaths = {
    MakeAssetPath("ui/hud_coin/misc_3__hud_coin1_1.png"),
    MakeAssetPath("ui/hud_coin/misc_3__hud_coin1_2.png"),
    MakeAssetPath("ui/hud_coin/misc_3__hud_coin1_3.png"),
};
static constexpr std::array<int, 4> kCoinAnimationFrames = {0, 1, 2, 1};
static constexpr int kCoinAnimationFrameTicks = 16;

// ─── 輔助：建立一個包有 Text 的 GameObject ────────────────────────────────
static std::shared_ptr<Util::GameObject> MakeTextObj(
    std::shared_ptr<Util::Text> text, float x, float y, float zIndex = 20.0f)
{
    auto obj = std::make_shared<Util::GameObject>(text, zIndex);
    obj->m_Transform.translation = {x, y};
    obj->SetVisible(true);
    return obj;
}

// ─── Init ─────────────────────────────────────────────────────────────────

void HUD::Init(Util::Renderer& renderer, const std::string& worldLabel, const std::string& playerName) {
    m_PlayerName = playerName;
    m_LastScore = -1;
    m_LastCoins = -1;
    m_LastTimeLeft = -2;

    // 分數區塊："MARIO" 標題 + 分數數值
    m_ScoreText = std::make_shared<Util::Text>(kFontPath, kFontSize, m_PlayerName + "\n000000", kWhite);
    m_ScoreObj  = MakeTextObj(m_ScoreText, kScoreX, kTopY);

    // 金幣區塊：先加金幣圖示，再是文字「x03」
    // 原版 HUD 金幣圖示約 8 NES 像素高（= 24 螢幕像素），對齊數值列（NES y=24..32）
    m_CoinAnimationTick = 0;
    m_CoinAnimationIndex = 0;
    for (std::size_t i = 0; i < kCoinFramePaths.size(); ++i) {
        auto coinImage = std::make_shared<Util::Image>(kCoinFramePaths[i]);
        m_CoinImageObjs[i] = std::make_shared<Util::GameObject>(coinImage, 20.0f);
        m_CoinImageObjs[i]->m_Transform.translation = {kCoinIconX, kTopY - kFontSize * 0.5f};
        const auto texSize = coinImage->GetSize();
        const float iconScale = (texSize.y > 0.0f) ? 24.0f / texSize.y : 1.5f;
        m_CoinImageObjs[i]->m_Transform.scale = {iconScale, iconScale};
        m_CoinImageObjs[i]->SetVisible(i == 0);
    }

    // 單行文字：垂直對齊數值列（兩行中心往下半個字高）
    m_CoinText  = std::make_shared<Util::Text>(kFontPath, kFontSize, "x00", kWhite);
    m_CoinObj   = MakeTextObj(m_CoinText, kCoinX, kTopY - kFontSize * 0.5f);

    auto worldText = std::make_shared<Util::Text>(kFontPath, kFontSize, "WORLD\n " + worldLabel, kWhite);
    m_WorldObj  = MakeTextObj(worldText, kWorldX, kTopY);

    // 時間區塊
    m_TimeText  = std::make_shared<Util::Text>(kFontPath, kFontSize, "TIME\n 400", kWhite);
    m_TimeObj   = MakeTextObj(m_TimeText, kTimeX, kTopY);

    // 全部加進渲染器
    renderer.AddChild(m_ScoreObj);
    for (const auto& coinImageObj : m_CoinImageObjs) {
        renderer.AddChild(coinImageObj);
    }
    renderer.AddChild(m_CoinObj);       // 金幣數字
    renderer.AddChild(m_WorldObj);
    renderer.AddChild(m_TimeObj);

    LOG_INFO("HUD initialized. Font: {}", kFontPath);
}

// ─── Update ───────────────────────────────────────────────────────────────

void HUD::Update(int score, int coins, int timeLeft) {
    ++m_CoinAnimationTick;
    if (m_CoinAnimationTick >= kCoinAnimationFrameTicks) {
        m_CoinAnimationTick = 0;
        m_CoinAnimationIndex = (m_CoinAnimationIndex + 1) %
                               static_cast<int>(kCoinAnimationFrames.size());
        const int visibleFrame = kCoinAnimationFrames[m_CoinAnimationIndex];
        for (int i = 0; i < static_cast<int>(m_CoinImageObjs.size()); ++i) {
            if (m_CoinImageObjs[i]) {
                m_CoinImageObjs[i]->SetVisible(i == visibleFrame);
            }
        }
    }

    // 分數：標題換行 + 6 位補零數值
    if (score != m_LastScore) {
        m_ScoreText->SetText(m_PlayerName + "\n" + FormatScore(score));
        m_LastScore = score;
    }

    // 金幣：「×」符號 + 2 位數
    if (coins != m_LastCoins) {
        std::ostringstream cs;
        cs << "x" << std::setw(2) << std::setfill('0') << std::min(coins, 99);
        m_CoinText->SetText(cs.str());
        m_LastCoins = coins;
    }

    // 時間（傳入 -1 時保留 TIME 標題，但不顯示數字）
    if (timeLeft < 0) {
        m_TimeObj->SetVisible(true);
        if (m_LastTimeLeft != -1) {
            m_TimeText->SetText("TIME\n    ");
            m_LastTimeLeft = -1;
        }
        return;
    }

    m_TimeObj->SetVisible(true);
    if (timeLeft != m_LastTimeLeft) {
        m_TimeText->SetText("TIME\n " + FormatTime(timeLeft));
        m_LastTimeLeft = timeLeft;
    }
}

// ─── 私有格式化工具 ────────────────────────────────────────────────────────

std::string HUD::FormatScore(int score) {
    std::ostringstream ss;
    ss << std::setw(6) << std::setfill('0') << std::min(score, 999999);
    return ss.str();
}

std::string HUD::FormatTime(int t) {
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << std::max(0, std::min(t, 999));
    return ss.str();
}
