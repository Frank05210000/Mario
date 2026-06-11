#include "FireFlowerItem.hpp"
#include "Player.hpp"
#include "AssetPath.hpp"
#include "Util/Logger.hpp"

FireFlowerItem::FireFlowerItem(glm::vec2 pos, const ThemeAssets& assets) : Item(pos) {
    m_Size = {TILE_SIZE, TILE_SIZE};

    std::vector<std::string> animPaths = {
        assets.Sprite("item/powerup/{theme}/flower/flower-1.png"),
        assets.Sprite("item/powerup/{theme}/flower/flower-2.png"),
        assets.Sprite("item/powerup/{theme}/flower/flower-3.png"),
        assets.Sprite("item/powerup/{theme}/flower/flower-4.png")
    };
    
    m_Animation = std::make_shared<Util::Animation>(animPaths, true, 100, true);
    SetDrawable(m_Animation);
    SetZIndex(0.5f); // 在方塊後方一點點

    // 初始速度 (往上冒)
    m_Velocity = {0.0f, -30.0f};
}

void FireFlowerItem::Update(float deltaTime) {
    if (m_State == ItemState::Collected) return;

    if (m_State == ItemState::Spawning) {
        // 冉冉升起
        m_Position.y += m_Velocity.y * deltaTime;
        
        // 如果整朵花都冒出方塊頂部了
        if (m_Position.y <= m_SpawnBaseY - m_Size.y) {
            m_Position.y = m_SpawnBaseY - m_Size.y;
            m_State = ItemState::Active;
            m_Velocity = {0.0f, 0.0f}; // 花不會動
            SetZIndex(6.0f); // 跑到前面來
        }
    }
}

void FireFlowerItem::OnCollect(Player* player) {
    if (m_State != ItemState::Active) return;

    LOG_INFO("FireFlower collected by Player!");
    m_State = ItemState::Collected;
    
    // 將瑪利歐型態改變為 FIRE
    player->SetForm(Player::Form::FIRE);
}
