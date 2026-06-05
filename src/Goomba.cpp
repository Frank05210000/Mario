#include "Goomba.hpp"

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

Goomba::Goomba(float startX, float startY, const std::string& theme) {
    const std::string spriteTheme = (theme == "underground") ? "underground" : "ground";
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE};      // 世界尺寸：16x16
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    SetDrawable(std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Goomba/" + spriteTheme + "/normal/walk/walk-1.png")));
}

void Goomba::Stomp() {
    // 栗寶寶：被踩就直接消失
    SetAlive(false);
}
