#include "Map.hpp"

#include <filesystem>

#include "AssetCatalog.hpp"
#include "Core/Context.hpp"
#include "Util/Image.hpp"

namespace {
std::string MakeAssetPath(const std::string& relativePath) {
    const auto assetRoot =
        std::filesystem::path(RESOURCE_DIR).parent_path() / "Asset";
    return (assetRoot / relativePath).string();
}

std::string MakeDataPath(const std::string& relativePath) {
    const auto projectRoot =
        std::filesystem::path(RESOURCE_DIR).parent_path();
    return (projectRoot / relativePath).string();
}

std::string ResolveProjectPath(const std::string& path) {
    const std::filesystem::path inputPath(path);
    if (inputPath.is_absolute()) {
        return inputPath.string();
    }

    const auto projectRoot =
        std::filesystem::path(RESOURCE_DIR).parent_path();
    return (projectRoot / inputPath).string();
}
}

Map::Map()
    : m_Background(
          MakeAssetPath("image/stage/1-1.png"),
          {0.0F, 0.0F},
          {6752.0F, 480.0F},
          SDL_Rect {0, 0, 6752, 480}) {}

void Map::load() {
    m_LevelData = m_LevelLoader.LoadFromFile(MakeDataPath("data/1-1.json"));
    m_Background = Background(
        ResolveProjectPath(m_LevelData.backgroundImagePath),
        {0.0F, 0.0F},
        {static_cast<float>(m_LevelData.levelWidth), static_cast<float>(m_LevelData.levelHeight)},
        SDL_Rect {0, 0, m_LevelData.levelWidth, m_LevelData.levelHeight});
    m_Background.load();
    BuildObjectGameObjects();

    for (const auto& object : m_LevelData.objects) {
        if (object.type == ObjectType::SpawnPoint) {
            m_PlayerSpawn = {object.x, object.y};
            break;
        }
    }
}

void Map::render(float cameraX) {
    m_Background.render(cameraX);
    UpdateObjectTransforms(cameraX);
}

std::shared_ptr<Util::GameObject> Map::GetBackgroundObject() const {
    return m_Background.GetGameObject();
}

std::vector<const LevelObject*> Map::GetSolidObjects() const {
    std::vector<const LevelObject*> objects;
    objects.reserve(m_LevelData.objects.size());

    for (const auto& object : m_LevelData.objects) {
        if (object.solid) {
            objects.push_back(&object);
        }
    }

    return objects;
}

void Map::BuildObjectGameObjects() {
    m_ObjectGameObjects.clear();

    for (const auto& object : m_LevelData.objects) {
        if (!object.visible || !object.render.has_value()) {
            continue;
        }

        const auto spritePath = AssetCatalog::GetLevelObjectSprite(object.type);
        if (spritePath.empty()) {
            continue;
        }

        auto gameObject = std::make_shared<Util::GameObject>();
        gameObject->SetDrawable(std::make_shared<Util::Image>(spritePath));
        gameObject->SetZIndex(5.0F);
        gameObject->SetPivot({-(object.width * 0.5F), object.height * 0.5F});

        const auto drawableSize = gameObject->GetScaledSize();
        if (drawableSize.x > 0.0F && drawableSize.y > 0.0F) {
            gameObject->m_Transform.scale = {
                object.width / drawableSize.x,
                object.height / drawableSize.y,
            };
        }

        m_ObjectGameObjects.push_back(gameObject);
    }
}

void Map::UpdateObjectTransforms(float cameraX) {
    std::size_t objectIndex = 0;
    for (const auto& object : m_LevelData.objects) {
        if (!object.visible || !object.render.has_value()) {
            continue;
        }

        if (objectIndex >= m_ObjectGameObjects.size()) {
            break;
        }

        m_ObjectGameObjects[objectIndex]->m_Transform.translation =
            ToScenePosition(object.x, object.y, cameraX);
        ++objectIndex;
    }
}

glm::vec2 Map::ToScenePosition(float worldX, float worldY, float cameraX) const {
    const auto context = Core::Context::GetInstance();
    const auto windowHalfWidth =
        static_cast<float>(context->GetWindowWidth()) * 0.5F;
    const auto windowHalfHeight =
        static_cast<float>(context->GetWindowHeight()) * 0.5F;

    return {
        worldX - cameraX - windowHalfWidth,
        windowHalfHeight - worldY,
    };
}
