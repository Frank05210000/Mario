#include "Camera.hpp"

#include "Core/Context.hpp"
#include "GameConstants.hpp"

void Camera::Update(float targetX, float levelWidth) {
    const auto  ctx     = Core::Context::GetInstance();
    const float windowW = static_cast<float>(ctx->GetWindowWidth());

    const float viewW = windowW / GAME_SCALE;
    const float maxX  = std::max(0.0f, levelWidth - viewW);

    float desiredX = targetX - viewW * 0.5f;      // 玩家置中
    desiredX = std::clamp(desiredX, 0.0f, maxX);  // 先夾地圖邊界
    m_X = std::max(m_X, desiredX);                // 棘輪：只進不退
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
