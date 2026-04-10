#include "Player.hpp"

#include "AssetCatalog.hpp"
#include "CollisionSystem.h"
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

void Player::update(const Map& map) {
    const auto previousX = m_Position.x;

    if (Util::Input::IsKeyPressed(Util::Keycode::LEFT)) {
        m_Position.x -= m_Speed;
    }

    if (Util::Input::IsKeyPressed(Util::Keycode::RIGHT)) {
        m_Position.x += m_Speed;
    }

    ResolveHorizontalCollisions(map, previousX);

    if (m_OnGround && !HasGroundSupport(map)) {
        m_OnGround = false;
    }

    ApplyJump();

    const auto previousY = m_Position.y;
    ApplyGravity();
    ResolveVerticalCollisions(map, previousY);
    ClampToWorldBounds(map.GetWorldWidth());
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

void Player::SetSpawnPosition(Vec2 position) {
    m_Position = position;
    m_VelocityY = 0.0F;
}

void Player::ClampToWorldBounds(float worldWidth) {
    if (m_Position.x < 0.0F) {
        m_Position.x = 0.0F;
    }

    const auto maxX = worldWidth - m_Size.x;
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

void Player::ResolveHorizontalCollisions(const Map& map, float previousX) {
    const Rect playerBox {m_Position.x, m_Position.y, m_Size.x, m_Size.y};
    const auto movingRight = m_Position.x > previousX;
    const auto movingLeft = m_Position.x < previousX;

    for (const auto* object : map.GetSolidObjects()) {
        if (!CollisionSystem::Intersects(playerBox, object->collider)) {
            continue;
        }

        if (movingRight) {
            m_Position.x = object->collider.x - m_Size.x;
        } else if (movingLeft) {
            m_Position.x = object->collider.x + object->collider.w;
        }
    }
}

void Player::ResolveVerticalCollisions(const Map& map, float previousY) {
    const Rect currentBox {m_Position.x, m_Position.y, m_Size.x, m_Size.y};
    const Rect previousBox {m_Position.x, previousY, m_Size.x, m_Size.y};
    bool landed = false;

    for (const auto* object : map.GetSolidObjects()) {
        if (!CollisionSystem::Intersects(currentBox, object->collider)) {
            continue;
        }

        const auto previousBottom = previousBox.y + previousBox.h;
        const auto currentBottom = currentBox.y + currentBox.h;
        const auto previousTop = previousBox.y;
        const auto currentTop = currentBox.y;

        if (m_VelocityY >= 0.0F &&
            previousBottom <= object->collider.y &&
            currentBottom >= object->collider.y) {
            m_Position.y = object->collider.y - m_Size.y;
            m_VelocityY = 0.0F;
            landed = true;
            continue;
        }

        if (m_VelocityY < 0.0F &&
            previousTop >= object->collider.y + object->collider.h &&
            currentTop <= object->collider.y + object->collider.h) {
            m_Position.y = object->collider.y + object->collider.h;
            m_VelocityY = 0.0F;
        }
    }

    m_OnGround = landed || HasGroundSupport(map);
}

bool Player::HasGroundSupport(const Map& map) const {
    const Rect feetBox {
        m_Position.x + 2.0F,
        m_Position.y + m_Size.y,
        m_Size.x - 4.0F,
        2.0F,
    };

    for (const auto* object : map.GetSolidObjects()) {
        if (CollisionSystem::Intersects(feetBox, object->collider)) {
            return true;
        }
    }
    return false;
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
