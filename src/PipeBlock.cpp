#include "PipeBlock.hpp"

#include "AssetPath.hpp"
#include "Camera.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"
#include <glm/gtc/constants.hpp>

PipeBlock::PipeBlock(glm::vec2 position,
                     glm::vec2 size,
                     const std::string& opening,
                     bool enterable,
                     const std::string& targetLevel,
                     const std::string& exitToLevel,
                     glm::vec2 targetSpawn,
                     bool hasTargetSpawn,
                     const std::string& theme)
    : Block(position, size),
      m_Enterable(enterable),
      m_Opening(opening),
      m_TargetLevel(targetLevel),
      m_ExitToLevel(exitToLevel),
      m_TargetSpawn(targetSpawn),
      m_HasTargetSpawn(hasTargetSpawn),
      m_Theme(theme) {

    const std::string topPath = "block/" + m_Theme + "/pipe/pipe_top/pipe_top.png";
    const std::string midPath = "block/" + m_Theme + "/pipe/pipe_mid/pipe_mid.png";
    const std::string leftPath = "block/" + m_Theme + "/pipe/pipe_left/pipe_left.png";
    const std::string rightPath = "block/" + m_Theme + "/pipe/pipe_right/pipe_right.png";

    auto topImg = std::make_shared<Util::Image>(MakeAssetPath(topPath));
    auto midImg = std::make_shared<Util::Image>(MakeAssetPath(midPath));
    auto leftImg = std::make_shared<Util::Image>(MakeAssetPath(leftPath));
    auto rightImg = std::make_shared<Util::Image>(MakeAssetPath(rightPath));

    const glm::vec2 verticalSegmentSize = {TILE_SIZE * 2.0f, TILE_SIZE};
    const glm::vec2 horizontalSegmentSize = {TILE_SIZE, TILE_SIZE * 2.0f};
    const glm::vec2 defaultScale = {GAME_SCALE, GAME_SCALE};
    const glm::vec2 horizontalCapScale = {GAME_SCALE, GAME_SCALE * 2.0f};

    if (m_Opening == "up") {
        int numSegments = static_cast<int>(size.y / TILE_SIZE);
        for (int i = 0; i < numSegments; ++i) {
            auto obj = std::make_shared<Util::GameObject>(i == 0 ? topImg : midImg, 1.0f);
            AddChild(obj);
            m_Segments.push_back({obj, glm::vec2(0.0f, i * TILE_SIZE), verticalSegmentSize, defaultScale, 0.0f});
        }
    } else if (m_Opening == "down") {
        int numSegments = static_cast<int>(size.y / TILE_SIZE);
        for (int i = 0; i < numSegments; ++i) {
            bool isTop = (i == numSegments - 1);
            auto obj = std::make_shared<Util::GameObject>(isTop ? topImg : midImg, 1.0f);
            AddChild(obj);
            m_Segments.push_back({obj, glm::vec2(0.0f, i * TILE_SIZE), verticalSegmentSize, defaultScale, glm::pi<float>()});
        }
    } else if (m_Opening == "left") {
        int numSegments = static_cast<int>(size.x / TILE_SIZE);
        for (int i = 0; i < numSegments; ++i) {
            const bool isOpening = (i == 0);
            auto obj = std::make_shared<Util::GameObject>(isOpening ? leftImg : midImg, 1.0f);
            AddChild(obj);
            m_Segments.push_back({
                obj,
                glm::vec2(i * TILE_SIZE, 0.0f),
                horizontalSegmentSize,
                isOpening ? horizontalCapScale : defaultScale,
                isOpening ? 0.0f : -glm::half_pi<float>()});
        }
    } else if (m_Opening == "right") {
        int numSegments = static_cast<int>(size.x / TILE_SIZE);
        for (int i = 0; i < numSegments; ++i) {
            const bool isOpening = (i == numSegments - 1);
            auto obj = std::make_shared<Util::GameObject>(isOpening ? rightImg : midImg, 1.0f);
            AddChild(obj);
            m_Segments.push_back({
                obj,
                glm::vec2(i * TILE_SIZE, 0.0f),
                horizontalSegmentSize,
                isOpening ? horizontalCapScale : defaultScale,
                isOpening ? 0.0f : glm::half_pi<float>()});
        }
    }
}

void PipeBlock::Draw(const Camera& camera) {
    for (auto& seg : m_Segments) {
        if (!seg.obj) continue;

        glm::vec2 segmentPos = m_Position + seg.offset;
        glm::vec2 centerPos = {
            segmentPos.x + seg.displaySize.x * 0.5f,
            segmentPos.y + seg.displaySize.y * 0.5f
        };

        seg.obj->m_Transform.translation = camera.WorldToScreen(centerPos);
        seg.obj->m_Transform.scale = seg.scale;
        seg.obj->m_Transform.rotation = seg.rotation;
    }
}
