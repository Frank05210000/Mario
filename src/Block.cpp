#include "Block.hpp"

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "Core/Context.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"

Block::Block(glm::vec2 position, glm::vec2 size)
    : m_Position(position), m_Size(size) {}

void Block::Update(float deltaTime) {
    (void)deltaTime;
}

void Block::Draw(const Camera& camera) {
    glm::vec2 centerPos = {
        m_Position.x + m_Size.x * 0.5f,
        m_Position.y + m_Size.y * 0.5f
    };
    m_Transform.translation = camera.WorldToScreen(centerPos);
}

void Block::SetSprite(const std::string& assetPath, float zIndex) {
    auto image = std::make_shared<Util::Image>(MakeAssetPath(assetPath));
    SetDrawable(image);
    SetZIndex(zIndex);

    const auto textureSize = image->GetSize();
    if (textureSize.x > 0.0f && textureSize.y > 0.0f) {
        m_Transform.scale = {
            (m_Size.x * GAME_SCALE) / textureSize.x,
            (m_Size.y * GAME_SCALE) / textureSize.y,
        };
    }
}


