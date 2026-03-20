#ifndef MAP_HPP
#define MAP_HPP

#include "Background.hpp"

class Map {
public:
    static constexpr glm::vec2 kWorldSize {6752.0F, 480.0F};

    Map();

    void load();
    void render(float cameraX);

    std::shared_ptr<Util::GameObject> GetBackgroundObject() const;

private:
    Background m_Background;
};

#endif
