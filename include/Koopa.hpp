#ifndef KOOPA_HPP
#define KOOPA_HPP

#include <memory>
#include "ThemeAssets.hpp"

#include "Enemy.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

/*
 * Koopa：烏龜
 *
 * 繼承 Enemy，實作 Stomp()。
 * 被踩到後縮進殼裡，停止走動；殼之後可以被踢走。
 * 火球或星星命中時進入翻轉死亡飛出動畫（Die() path）。
 * 縮殼靜止 ~5s 後會自動還原走路（wake-up）。
 *
 * Red 變體：若 Resources/Asset/enemy/Koopa/ground/red/ 目錄存在則載入紅色
 * 精靈，否則 fallback 使用綠色精靈。
 */
class Koopa : public Enemy {
public:
    enum class Variant { Green, Red };

    /* 建構子
     * startX / startY：出生的世界座標位置。
     * variant：顏色變體（Green 或 Red），預設 Green。
     * assets：主題資產解析器，用來載入對應主題的精靈。
     */
    Koopa(float startX,
          float startY,
          Variant variant = Variant::Green,
          const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    /* 每幀更新（覆寫 Enemy::Update）
     * 縮殼狀態下停止水平移動；縮殼 5s 後喚醒；其他時候和普通敵人一樣。
     */
    void Update(float deltaTime) override;

    /* 被踩：縮進殼裡
     * 不直接死亡，改成停止走路的縮殼狀態。
     */
    void Stomp() override;

    /*
     * 被踢（已在殼裡的狀態下再被踢）
     * 殼會朝踢的方向橫向快速滑行，可打到敵人
     */
    void Kick(bool kickLeft);

    /*
     * 翻轉死亡（被火球 / 星星命中）
     * 給一個向上的初速並讓重力拉它出畫面；期間非 Alive。
     */
    void Die(bool flipLeft);

    // 查詢是否在殼裡
    bool IsInShell() const { return m_InShell; }
    // 查詢殼是否在滑行中
    bool IsSliding() const { return m_IsSliding; }
    // 查詢是否正在翻轉死亡飛出
    bool IsDying() const { return m_IsDying; }
    Variant GetVariant() const { return m_Variant; }

    /* 覆寫 Draw：翻轉死亡中保持可見直到飛出畫面 */
    void Draw(const Camera& camera) override;

protected:
    void UpdateDrawable();
    void LoadSprites(const ThemeAssets& assets = ThemeAssets(Theme::Ground));

    std::shared_ptr<Util::Animation> m_WalkLeftAnim;
    std::shared_ptr<Util::Animation> m_WalkRightAnim;
    std::shared_ptr<Util::Image>     m_ShellImage;
    std::shared_ptr<Util::Image>     m_ShellFlipImage; // 翻轉殼（死亡用）

    bool m_InShell    = false;  // true = 已縮進殼裡
    bool m_IsSliding  = false;  // true = 殼被踢出，正在橫向滑行
    bool m_IsDying    = false;  // true = 翻轉死亡飛出中
    Variant m_Variant = Variant::Green;

    // 縮殼喚醒計時器（僅在縮殼非滑行時計時）
    float m_WakeUpTimer  = 0.0f;
    static constexpr float WAKE_UP_DELAY = 5.0f;  // 5 秒後喚醒
};

#endif
