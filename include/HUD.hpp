#ifndef HUD_HPP
#define HUD_HPP

#include <memory>
#include <string>

#include "Util/Text.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"
#include "Util/Color.hpp"
#include "Util/GameObject.hpp"

/*
 * HUD：遊戲狀態列
 *
 * 持有所有文字物件（分數、金幣、世界、時間），
 * 每幀接收最新數據並更新文字內容。
 *
 * 設計要點：
 *   - 使用螢幕固定座標（不受鏡頭影響）
 *   - 由 GameManager 持有（組合，has-a）
 *   - 文字物件需加入 Renderer 才會被渲染
 *
 * OOP：
 *   - HUD 不繼承任何人（純 UI 管理者，不是遊戲場景物件）
 */
class HUD {
public:
    /*
     * 初始化所有文字物件（建立 Text、設定字體與顏色）
     * 傳入 Renderer 是為了把文字物件加入渲染清單。
     *
     * @param renderer  PTSD 渲染器的參考
     */
    void Init(Util::Renderer& renderer,
              const std::string& worldLabel = "1-1",
              const std::string& playerName = "MARIO");

    /*
     * 每幀更新文字內容
     *
     * @param score     玩家當前分數
     * @param coins     玩家收集的金幣數
     * @param timeLeft  關卡剩餘時間（秒，-1 表示不顯示）
     */
    void Update(int score, int coins, int timeLeft = -1);

private:
    // 格式化分數：補零至 6 位（原版 NES 風格）
    static std::string FormatScore(int score);
    // 格式化時間：補零至 3 位
    static std::string FormatTime(int t);

    // ─── 各區塊的 GameObject 包裝 ─────────────────────────────────
    // Util::Text 是 Drawable，需包在 GameObject 才能加入 Renderer

    std::shared_ptr<Util::GameObject> m_ScoreObj;   // "MARIO\n000100"
    std::shared_ptr<Util::GameObject> m_CoinImageObj;  // 金幣圖示（在數字左側）
    std::shared_ptr<Util::GameObject> m_CoinObj;    // "×03"
    std::shared_ptr<Util::GameObject> m_WorldObj;   // "WORLD\n 1-2"
    std::shared_ptr<Util::GameObject> m_TimeObj;    // "TIME\n 300"

    // 存放對應的 Text Drawable，方便後續呼叫 SetText()
    std::shared_ptr<Util::Text> m_ScoreText;
    std::shared_ptr<Util::Text> m_CoinText;
    std::shared_ptr<Util::Text> m_TimeText;
    std::string m_PlayerName = "MARIO";
};

#endif
