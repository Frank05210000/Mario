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
    SetClippedDrawable(m_Animation); // 冒出期間沿方塊頂線裁切
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

    LOG_INFO("FireFlower collected by Player! (with transform animation)");
    m_State = ItemState::Collected;

    // 啟動升級變身動畫（全場凍結 ~1 秒）
    // 依原版行為：吃花時，SMALL → SUPER → FIRE 間所有變換都有動畫
    // 簡化：若尚未是 FIRE，就觸發動畫（from = 當前, to = FIRE）
    if (player->GetForm() != Player::Form::FIRE) {
        player->StartTransformAnimation(player->GetForm(), Player::Form::FIRE);
    }
    // 已經是 FIRE 則不重播動畫，維持 FIRE 不變
}
