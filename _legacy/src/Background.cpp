#include "Background.hpp"

#include "CroppedImage.hpp"
#include "Core/Context.hpp"

Background::Background(std::string assetPath,
                       glm::vec2 topLeft,
                       glm::vec2 size,
                       SDL_Rect cropRect)
    : m_AssetPath(std::move(assetPath)),
      m_TopLeft(topLeft),
      m_Size(size),
      m_CropRect(cropRect) {}

void Background::load() {
    m_Object = std::make_shared<Util::GameObject>();
    m_Object->SetDrawable(std::make_shared<CroppedImage>(m_AssetPath, m_CropRect));
    m_Object->SetZIndex(0.0F);
    m_Object->SetPivot({-(m_Size.x * 0.5F), m_Size.y * 0.5F});
}

void Background::render(float cameraX) {
    if (m_Object == nullptr) {
        return;
    }

    m_Object->m_Transform.translation = ToScenePosition(cameraX);
}

glm::vec2 Background::ToScenePosition(float cameraX) const {
    const auto context = Core::Context::GetInstance();
    const auto windowHalfWidth =
        static_cast<float>(context->GetWindowWidth()) * 0.5F;
    const auto windowHalfHeight =
        static_cast<float>(context->GetWindowHeight()) * 0.5F;

    return {
        m_TopLeft.x - cameraX - windowHalfWidth,
        windowHalfHeight - m_TopLeft.y,
    };
}
