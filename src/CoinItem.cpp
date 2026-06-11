#include "CoinItem.hpp"
#include "AssetPath.hpp"
#include "Util/Animation.hpp"
#include "Util/Logger.hpp"
#include <vector>

CoinItem::CoinItem(glm::vec2 pos, const ThemeAssets& assets) : Item(pos) {
    m_Size = {TILE_SIZE, TILE_SIZE};

    std::vector<std::string> paths = {
        assets.Sprite("item/coin/{theme}/coin-1.png"),
        assets.Sprite("item/coin/{theme}/coin-2.png"),
        assets.Sprite("item/coin/{theme}/coin-3.png")
    };

    m_CoinAnim = std::make_shared<Util::Animation>(paths, true, 100, true);
    SetDrawable(m_CoinAnim);
    SetZIndex(8.0f); // 金幣要在前面

    m_Velocity = {0.0f, -400.0f}; // 強烈的初速往上噴
    m_Gravity = 1500.0f;
    m_State = ItemState::Active; // 金幣一開始就受重力影響
}

void CoinItem::Update(float deltaTime) {
    if (m_State == ItemState::Collected) return;

    ApplyGravity(deltaTime);

    // 如果掉回產生的 Y 軸，代表蹦完了，自動消失
    if (m_Velocity.y > 0 && m_Position.y >= m_SpawnBaseY - m_Size.y) {
        LOG_INFO("Coin collected automatically!");
        m_State = ItemState::Collected;
    }
}

void CoinItem::OnCollect(Player* /*player*/) {
    // 方塊噴出的金幣不會被吃，是自動跑完動畫
}
