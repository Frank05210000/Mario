#include "FlagBlock.hpp"

#include "GameConstants.hpp"
#include "AssetPath.hpp"
#include "Camera.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

// 旗杆總高度（像素），由常數自動計算
static constexpr float POLE_HEIGHT = FLAG_POLE_TILES * TILE_SIZE;

FlagBlock::FlagBlock(glm::vec2 bottomPosition)
    : Block({bottomPosition.x, bottomPosition.y - POLE_HEIGHT}, {TILE_SIZE, POLE_HEIGHT}) {
    // 只顯示旗杆頂端的球（旗杆本體可之後另外疊加圖層）
    // TODO: Resources/Asset/item/flag/ 目前只有 ball.png，沒有旗幟 flag sprite。
    //       若後續取得 flag.png，請在此加入旗幟子 GameObject 並同步下移。
    SetSprite("item/flag/ball.png", 1.0f);

    // 修正 SetSprite 自動把 Y 縮放成 m_Size.y (POLE_HEIGHT) 的問題，
    // 強制把球的大小縮放回 1 格 TILE_SIZE
    if (auto img = std::dynamic_pointer_cast<Util::Image>(m_Drawable)) {
        const auto textureSize = img->GetSize();
        if (textureSize.x > 0.0f && textureSize.y > 0.0f) {
            m_Transform.scale = {
                (TILE_SIZE * GAME_SCALE) / textureSize.x,
                (TILE_SIZE * GAME_SCALE) / textureSize.y,
            };
        }
    }

    // 初始化球下降動畫：球在杆頂（偏移 = TILE_SIZE，避開最頂端）
    m_BallOffsetY = TILE_SIZE;
    // 下降目標：杆高 - 一格（到達杆底附近）
    m_BallTargetY = POLE_HEIGHT - TILE_SIZE;
}

void FlagBlock::StartDescent() {
    if (!m_IsDescending) {
        m_IsDescending = true;
        LOG_INFO("FlagBlock: ball descent started.");
    }
}

void FlagBlock::Update(float dt) {
    if (!m_IsDescending) return;

    if (m_BallOffsetY < m_BallTargetY) {
        m_BallOffsetY += DESCENT_SPEED * dt;
        if (m_BallOffsetY >= m_BallTargetY) {
            m_BallOffsetY = m_BallTargetY;
            m_IsDescending = false;
            LOG_INFO("FlagBlock: ball descent finished.");
        }
    }
}

void FlagBlock::Draw(const Camera& camera) {
    // 覆寫 Draw，讓旗子的位置隨 m_BallOffsetY 動態更新。
    // m_Position.y 是碰撞範圍頂點（杆頂），加上偏移後得到旗子的世界 Y。
    // FLAG_SPRITE_DROP：整體再往下微調，讓旗子貼齊杆底（實機校正值）。
    constexpr float FLAG_SPRITE_DROP = 8.0f;
    glm::vec2 centerPos = {
        m_Position.x,
        m_Position.y + m_BallOffsetY + FLAG_SPRITE_DROP
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);
}


/*
 * 計算玩家接觸旗杆的得分
 *
 * ratio = (旗杆底部 Y - 玩家 Y) / 旗杆總高
 *         → ratio = 1.0  代表最高點（旗頂）
 *         → ratio = 0.0  代表最低點（旗底）
 *
 * 分數區間 (對應原版 NES)：
 *   > 0.8  → 5000
 *   > 0.6  → 2000
 *   > 0.4  → 800
 *   > 0.2  → 400
 *   else   → 100
 */
int FlagBlock::GetContactScore(float playerY) const {
    const float poleTop    = m_Position.y;              // 旗杆頂端 Y
    const float poleBottom = m_Position.y + POLE_HEIGHT; // 旗杆底端 Y

    // 夾制：防止除以零或超出範圍
    const float clampedY = std::max(poleTop, std::min(playerY, poleBottom));
    const float ratio = (poleBottom - clampedY) / POLE_HEIGHT;

    LOG_INFO("Flag contact: playerY={:.1f}  poleTop={:.1f}  ratio={:.2f}",
             playerY, poleTop, ratio);

    if (ratio > 0.8f) return 5000;
    if (ratio > 0.6f) return 2000;
    if (ratio > 0.4f) return  800;
    if (ratio > 0.2f) return  400;
    return 100;
}
