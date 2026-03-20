#ifndef BACKGROUND_HPP
#define BACKGROUND_HPP

#include <memory>
#include <string>

#include <SDL_rect.h>

#include "Util/GameObject.hpp"
#include "glm/vec2.hpp"

class Background {
public:
    Background(std::string assetPath,
               glm::vec2 topLeft,
               glm::vec2 size,
               SDL_Rect cropRect);

    void load();
    void render(float cameraX);

    std::shared_ptr<Util::GameObject> GetGameObject() const { return m_Object; }

private:
    glm::vec2 ToScenePosition(float cameraX) const;

    std::string m_AssetPath;
    glm::vec2 m_TopLeft;
    glm::vec2 m_Size;
    SDL_Rect m_CropRect;
    std::shared_ptr<Util::GameObject> m_Object;
};

#endif
