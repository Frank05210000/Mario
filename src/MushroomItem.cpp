#include "MushroomItem.hpp"
#include "Player.hpp"
#include "AssetPath.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

MushroomItem::MushroomItem(glm::vec2 pos, const std::string& theme) : Item(pos) {
    const std::string spriteTheme = (theme == "underground") ? "underground" : "ground";
    m_Size = {TILE_SIZE, TILE_SIZE};
    auto image = std::make_shared<Util::Image>(
        MakeAssetPath("item/powerup/" + spriteTheme + "/mushroom/mushroom.png"));
    SetDrawable(image);
    SetZIndex(0.5f); // 在方塊後方一點點

    // 初始速度 (往上冒)
    m_Velocity = {0.0f, -30.0f};
}

void MushroomItem::Update(float deltaTime) {
    if (m_State == ItemState::Collected) return;

    if (m_State == ItemState::Spawning) {
        // 冉冉升起
        m_Position.y += m_Velocity.y * deltaTime;
        
        // 如果整顆香菇都冒出方塊頂部了
        if (m_Position.y <= m_SpawnBaseY - m_Size.y) {
            m_Position.y = m_SpawnBaseY - m_Size.y;
            m_State = ItemState::Active;
            m_Velocity = {50.0f, 0.0f}; // 開始往右走
            SetZIndex(6.0f); // 跑到前面來
        }
    } else if (m_State == ItemState::Active) {
        // 重力與走路
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
    }
}

void MushroomItem::OnCollect(Player* player) {
    if (m_State != ItemState::Active) return;

    LOG_INFO("Mushroom collected! Player -> SUPER");
    m_State = ItemState::Collected;
    // 只在SMALL時升級，已經是SUPER或FIRE就不變
    if (player->GetForm() == Player::Form::SMALL) {
        player->SetForm(Player::Form::SUPER);
    }
}
