#include "Map.hpp"

#include <filesystem>

namespace {
std::string MakeAssetPath(const std::string& relativePath) {
    const auto assetRoot =
        std::filesystem::path(RESOURCE_DIR).parent_path() / "Asset";
    return (assetRoot / relativePath).string();
}
}

Map::Map()
    : m_Background(
          MakeAssetPath("image/stage/1-1.png"),
          {0.0F, 0.0F},
          {6752.0F, 480.0F},
          SDL_Rect {0, 0, 6752, 480}) {}

void Map::load() {
    m_Background.load();
}

void Map::render(float cameraX) {
    m_Background.render(cameraX);
}

std::shared_ptr<Util::GameObject> Map::GetBackgroundObject() const {
    return m_Background.GetGameObject();
}
