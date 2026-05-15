#include "Camera.hpp"

#include "Core/Context.hpp"
#include "GameConstants.hpp"

void Camera::Update(float targetX, float levelWidth) {
    const auto  ctx     = Core::Context::GetInstance();
    const float windowW = static_cast<float>(ctx->GetWindowWidth());

    // 鏡頭中心對準追蹤目標（玩家）。因為畫面被放大 GAME_SCALE 倍，所以實體視窗寬度對應的世界寬度要除以 GAME_SCALE
    const float viewWorldWidth = windowW / GAME_SCALE;
    m_X = targetX - viewWorldWidth * 0.5f;

    // 夾制：不能超出地圖左邊
    if (m_X < 0.0f) m_X = 0.0f;

    // 夾制：不能超出地圖右邊
    const float maxX = levelWidth - viewWorldWidth;
    if (maxX > 0.0f && m_X > maxX) m_X = maxX;
}

float Camera::GetViewWorldWidth() const {
    const auto  ctx     = Core::Context::GetInstance();
    const float windowW = static_cast<float>(ctx->GetWindowWidth());
    return windowW / GAME_SCALE;
}

glm::vec2 Camera::WorldToScreen(glm::vec2 worldPos) const {
    const auto context = Core::Context::GetInstance();
    const float halfW = static_cast<float>(context->GetWindowWidth()) * 0.5f;
    const float halfH = static_cast<float>(context->GetWindowHeight()) * 0.5f;

    // 原點 (m_X) 對齊畫面極左，Y 軸原點在上方
    // PTSD 要求座標置於中心，Y 向上為正
    // 所以 (worldPos.x - m_X) 取相對距離後放大
    return {
        (worldPos.x - m_X) * GAME_SCALE - halfW,
        halfH - (worldPos.y * GAME_SCALE)
    };
}
