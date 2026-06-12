#include "StarmanItem.hpp"

#include <vector>

#include "AssetPath.hpp"
#include "Player.hpp"
#include "Util/Animation.hpp"
#include "Util/Logger.hpp"

StarmanItem::StarmanItem(glm::vec2 pos, const ThemeAssets& assets) : Item(pos) {
    m_Size = {TILE_SIZE, TILE_SIZE};
    m_Animation = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            assets.Sprite("item/powerup/{theme}/star/star-1.png"),
            assets.Sprite("item/powerup/{theme}/star/star-2.png"),
            assets.Sprite("item/powerup/{theme}/star/star-3.png"),
            assets.Sprite("item/powerup/{theme}/star/star-4.png"),
        },
        true,
        100,
        true);
    SetDrawable(m_Animation);
    SetZIndex(0.5f);
    m_Velocity = {0.0f, -30.0f};
}

void StarmanItem::Update(float deltaTime) {
    if (m_State == ItemState::Collected) return;

    if (m_State == ItemState::Spawning) {
        m_Position.y += m_Velocity.y * deltaTime;
        if (m_Position.y <= m_SpawnBaseY - m_Size.y) {
            m_Position.y = m_SpawnBaseY - m_Size.y;
            m_State = ItemState::Active;
            m_Velocity = {65.0f, -170.0f};
            SetZIndex(6.0f);
        }
    } else if (m_State == ItemState::Active) {
        ApplyGravity(deltaTime);
        m_Position.x += m_Velocity.x * deltaTime;
    }
}

void StarmanItem::OnCollect(Player* player) {
    if (m_State != ItemState::Active) return;
    LOG_INFO("Starman collected.");
    m_State = ItemState::Collected;
    if (player) {
        player->ActivateStarInvincibility();
    }
}
