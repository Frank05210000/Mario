#include "BrickDebris.hpp"

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

BrickDebris::BrickDebris(glm::vec2 position, glm::vec2 velocity, const std::string& theme)
    : m_Position(position), m_Velocity(velocity) {
    auto image = std::make_shared<Util::Image>(
        MakeAssetPath("block/" + theme + "/brick_piece/brick_piece.png"));
    SetDrawable(image);
    SetZIndex(6.0f);
    m_Transform.scale = {GAME_SCALE, GAME_SCALE};
    SetVisible(true);
}

void BrickDebris::Update(float deltaTime) {
    if (m_Lifetime <= 0.0f) return;

    m_Lifetime -= deltaTime;
    m_Velocity.y += m_Gravity * deltaTime;
    m_Position += m_Velocity * deltaTime;

    if (m_Lifetime <= 0.0f) {
        SetVisible(false);
    }
}

void BrickDebris::Draw(const Camera& camera) {
    const glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f,
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);
}
