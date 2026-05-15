#ifndef MAP_HPP
#define MAP_HPP

#include <memory>
#include <vector>

#include "Background.hpp"
#include "Entity.hpp"
#include "LevelLoader.h"
#include "Util/GameObject.hpp"

class Map {
public:
    Map();

    void load();
    void render(float cameraX);

    std::shared_ptr<Util::GameObject> GetBackgroundObject() const;
    const std::vector<std::shared_ptr<Util::GameObject>>& GetObjectGameObjects() const {
        return m_ObjectGameObjects;
    }
    std::vector<const LevelObject*> GetSolidObjects() const;
    Vec2 GetPlayerSpawn() const { return m_PlayerSpawn; }
    float GetWorldWidth() const { return static_cast<float>(m_LevelData.levelWidth); }
    float GetWorldHeight() const { return static_cast<float>(m_LevelData.levelHeight); }

private:
    void BuildObjectGameObjects();
    void UpdateObjectTransforms(float cameraX);
    glm::vec2 ToScenePosition(float worldX, float worldY, float cameraX) const;

private:
    LevelLoader m_LevelLoader;
    LevelData m_LevelData;
    Background m_Background;
    std::vector<std::shared_ptr<Util::GameObject>> m_ObjectGameObjects;
    Vec2 m_PlayerSpawn {240.0F, 353.92F};
};

#endif
