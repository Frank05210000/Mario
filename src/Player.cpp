#include "Player.hpp"

#include "AssetCatalog.hpp"
#include "Core/Context.hpp"
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

#include "Map.hpp"

Player::Player() {
    m_Position = {240.0F, 353.92F};
    m_Size = {32.0F, 64.0F};
    m_Form = Form::SUPER;
    m_OnGround = true;
}

void Player::update() {
    if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
        m_Position.x -= m_Speed;
    }

    if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
        m_Position.x += m_Speed;
    }

    ApplyJump();
    ApplyGravity();
    ResolveGroundCollision();
    ClampToWorldBounds();
}

void Player::render(float cameraX) {
    EnsureGameObject();
    m_Object->m_Transform.translation = ToScenePosition(cameraX);
}

void Player::ClampToCameraBounds(float cameraX) {
    if (m_Position.x < cameraX) {
        m_Position.x = cameraX;
    }
}

void Player::ClampToWorldBounds() {
    if (m_Position.x < 0.0F) {
        m_Position.x = 0.0F;
    }

    const auto maxX = Map::kWorldSize.x - m_Size.x;
    if (m_Position.x > maxX) {
        m_Position.x = maxX;
    }
}

void Player::ApplyJump() {
    if (!m_OnGround) {
        return;
    }

    if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
        m_VelocityY = -m_JumpStrength;
        m_OnGround = false;
    }
}

void Player::ApplyGravity() {
    if (m_OnGround) {
        return;
    }

    m_VelocityY += m_Gravity;
    m_Position.y += m_VelocityY;
}

void Player::ResolveGroundCollision() {
    if (m_Position.y + m_Size.y < m_GroundY) {
        return;
    }

    m_Position.y = m_GroundY - m_Size.y;
    m_VelocityY = 0.0F;
    m_OnGround = true;
}

glm::vec2 Player::ToScenePosition(float cameraX) const {
    const auto context = Core::Context::GetInstance();
    const auto windowHalfWidth =
        static_cast<float>(context->GetWindowWidth()) * 0.5F;
    const auto windowHalfHeight =
        static_cast<float>(context->GetWindowHeight()) * 0.5F;

    return {
        m_Position.x - cameraX - windowHalfWidth,
        windowHalfHeight - m_Position.y,
    };
}

void Player::EnsureGameObject() {
    if (m_Object != nullptr) {
        return;
    }

    m_Object = std::make_shared<Util::GameObject>();
    m_Object->SetDrawable(std::make_shared<Util::Image>(
        AssetCatalog::GetPlayerSprite(m_Form)));
    m_Object->SetZIndex(10.0F);
    m_Object->SetPivot({-(m_Size.x * 0.5F), m_Size.y * 0.5F});
}
