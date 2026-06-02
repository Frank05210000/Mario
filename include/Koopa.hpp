#ifndef KOOPA_HPP
#define KOOPA_HPP

#include <memory>

#include "Enemy.hpp"
#include "Util/Animation.hpp"
#include "Util/Image.hpp"

/*
 * Koopa：烏龜
 *
 * 繼承 Enemy，實作 Stomp()。
 * 被踩到後縮進殼裡，停止走動；殼之後可以被踢走。
 *
 * 行走邏輯沿用 Enemy::Update()，縮殼後覆寫讓它停下來。
 */
class Koopa : public Enemy {
public:
    enum class Variant { Green, Red };

    /* 建構子
     * startX / startY：出生的世界座標位置。
     */
    Koopa(float startX, float startY, Variant variant = Variant::Green);

    /* 每幀更新（覆寫 Enemy::Update）
     * 縮殼狀態下停止水平移動；其他時候和普通敵人一樣。
     */
    void Update(float deltaTime) override;

    /* 被踩：縮進殼裡
     * 不直接死亡，改成停止走路的縮殼狀態。
     * （之後可以擴充：被踢之後殼會滑行）
     */
    void Stomp() override;

    /*
     * 被踢（已在殼裡的狀態下再被踢）
     * 殼會朝踢的方向橫向快速滑行，可打到敋人
     */
    void Kick(bool kickLeft);

    // 查詢是否在殼裡
    bool IsInShell() const { return m_InShell; }
    // 查詢殼是否在滑行中
    bool IsSliding() const { return m_IsSliding; }
    Variant GetVariant() const { return m_Variant; }

protected:
    void UpdateDrawable();
    void LoadSprites(const std::string& theme = "ground");

    std::shared_ptr<Util::Animation> m_WalkLeftAnim;
    std::shared_ptr<Util::Animation> m_WalkRightAnim;
    std::shared_ptr<Util::Image> m_ShellImage;

    bool m_InShell   = false;  // true = 已縮進殼裡
    bool m_IsSliding = false;  // true = 殼被踢出，正在橫向滑行
    Variant m_Variant = Variant::Green;
};

#endif
