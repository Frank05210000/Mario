#include "OneUpMushroomItem.hpp"

#include "AssetPath.hpp"
#include "Player.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

OneUpMushroomItem::OneUpMushroomItem(glm::vec2 pos) : Item(pos) {
    m_Size = {TILE_SIZE, TILE_SIZE};
    SetDrawable(std::make_shared<Util::Image>(MakeAssetPath("item/powerup/mushroom/mushroom.png")));
    SetZIndex(0.5f);
    m_Velocity = {0.0f, -30.0f};
}

void OneUpMushroomItem::Update(float deltaTime) {
    if (m_State == ItemState::Collected) return;

    if (m_State == ItemState::Spawning) {
        m_Position.y += m_Velocity.y * deltaTime;
        if (m_Position.y <= m_SpawnBaseY - m_Size.y) {
            m_Position.y = m_SpawnBaseY - m_Size.y;
            m_State = ItemState::Active;
            m_Velocity = {50.0f, 0.0f};
            SetZIndex(6.0f);
        }
    } else if (m_State == ItemState::Active) {
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
    }
}

void OneUpMushroomItem::OnCollect(Player* /*player*/) {
    if (m_State != ItemState::Active) return;
    LOG_INFO("1-Up mushroom collected.");
    m_State = ItemState::Collected;
}
