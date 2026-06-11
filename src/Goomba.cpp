#include "Goomba.hpp"

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

Goomba::Goomba(float startX, float startY, const ThemeAssets& assets) {
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE};
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    SetDrawable(std::make_shared<Util::Image>(
        assets.Sprite("enemy/Goomba/{theme}/normal/walk/walk-1.png")));
}

void Goomba::Stomp() {
    // 栗寶寶：被踩就直接消失
    SetAlive(false);
}
