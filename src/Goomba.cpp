#include "Goomba.hpp"

#include "AssetPath.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

Goomba::Goomba(float startX, float startY) {
    m_Position = {startX, startY};
    m_Size     = {TILE_SIZE, TILE_SIZE};      // 世界尺寸：16x16
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};

    SetDrawable(std::make_shared<Util::Image>(
        MakeAssetPath("enemy/Goomba/ground/normal/walk/walk-1.png")));
}

void Goomba::Stomp() {
    // 栗寶寶：被踩就直接消失
    SetAlive(false);
}
