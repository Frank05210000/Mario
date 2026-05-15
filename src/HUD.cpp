#include "HUD.hpp"

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
static constexpr int   kFontSize   = 16;

// ─── 文字顏色（原版 NES：純白）────────────────────────────────────────────
static const Util::Color kWhite = Util::Color(255, 255, 255);

// ─── 螢幕佈局常數（PTSD 座標：原點在畫面中心，Y 向上為正）────────────────
// 視窗尺寸由 PTSD/config.hpp 定義；目前 960×720，半寬＝480，半高＝360
static constexpr float kHalfW  = WINDOW_WIDTH  / 2.0f;
static constexpr float kHalfH  = WINDOW_HEIGHT / 2.0f;
static constexpr float kTopY   = kHalfH  - 20.0f;       // 頂部留 20px 邊距

// 依照原版 NES 水平排列：MARIO | COIN | WORLD | TIME
static constexpr float kScoreX = -kHalfW + 110.0f;
static constexpr float kCoinX  = -kHalfW + 360.0f;
static constexpr float kWorldX = -kHalfW + 600.0f;
static constexpr float kTimeX  =  kHalfW - 120.0f;

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

void HUD::Init(Util::Renderer& renderer) {
    // 分數區塊："MARIO" 標題 + 分數數值
    m_ScoreText = std::make_shared<Util::Text>(kFontPath, kFontSize, "MARIO\n000000", kWhite);
    m_ScoreObj  = MakeTextObj(m_ScoreText, kScoreX, kTopY);

    // 金幣區塊："×" + 金幣數
    m_CoinText  = std::make_shared<Util::Text>(kFontPath, kFontSize, "x00", kWhite);
    m_CoinObj   = MakeTextObj(m_CoinText, kCoinX, kTopY);

    // 世界區塊（靜態，固定為 1-1）
    auto worldText = std::make_shared<Util::Text>(kFontPath, kFontSize, "WORLD\n 1-1", kWhite);
    m_WorldObj  = MakeTextObj(worldText, kWorldX, kTopY);

    // 時間區塊
    m_TimeText  = std::make_shared<Util::Text>(kFontPath, kFontSize, "TIME\n 400", kWhite);
    m_TimeObj   = MakeTextObj(m_TimeText, kTimeX, kTopY);

    // 全部加進渲染器
    renderer.AddChild(m_ScoreObj);
    renderer.AddChild(m_CoinObj);
    renderer.AddChild(m_WorldObj);
    renderer.AddChild(m_TimeObj);

    LOG_INFO("HUD initialized. Font: {}", kFontPath);
}

// ─── Update ───────────────────────────────────────────────────────────────

void HUD::Update(int score, int coins, int timeLeft) {
    // 分數：標題換行 + 6 位補零數值
    m_ScoreText->SetText("MARIO\n" + FormatScore(score));

    // 金幣：「×」符號 + 2 位數
    std::ostringstream cs;
    cs << "x" << std::setw(2) << std::setfill('0') << std::min(coins, 99);
    m_CoinText->SetText(cs.str());

    // 時間（傳入 -1 時顯示空）
    if (timeLeft >= 0) {
        m_TimeText->SetText("TIME\n " + FormatTime(timeLeft));
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
