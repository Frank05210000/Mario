#include "LevelCoinItem.hpp"

#include <vector>

#include "AssetPath.hpp"
#include "Player.hpp"
#include "Util/Animation.hpp"
#include "Util/Logger.hpp"

LevelCoinItem::LevelCoinItem(glm::vec2 pos, const std::string& theme) : Item(pos) {
    const std::string spriteTheme = (theme == "underground") ? "underground" : "ground";
    m_Size = {TILE_SIZE, TILE_SIZE};
    m_State = ItemState::Active;
    m_Velocity = {0.0f, 0.0f};
    m_CoinAnim = std::make_shared<Util::Animation>(
        std::vector<std::string>{
            MakeAssetPath("item/coin/" + spriteTheme + "/coin-1.png"),
            MakeAssetPath("item/coin/" + spriteTheme + "/coin-2.png"),
            MakeAssetPath("item/coin/" + spriteTheme + "/coin-3.png"),
        },
        true,
        120,
        true);
    SetDrawable(m_CoinAnim);
    SetZIndex(5.0f);
}

void LevelCoinItem::Update(float deltaTime) {
    (void)deltaTime;
}

void LevelCoinItem::OnCollect(Player* /*player*/) {
    if (m_State != ItemState::Active) return;
    LOG_INFO("Level coin collected.");
    m_State = ItemState::Collected;
}
